#pragma once
#include "FuncInfo.hh"
#include "Function.h"
#include "Pass.hh"
#include "PostDominator.hh"
#include "errorcode.hh"
#include "logging.hpp"

#include <deque>
#include <unordered_map>
#include <unordered_set>

/**
 * 更加激进的死代码消除，用以消除空循环，会改写控制流，将无用块结尾的分支指令替换为跳转到有用块的跳转指令
 **/
class ADCE : public Pass {
  public:
    ADCE(Module *m) : Pass(m), func_info(std::make_shared<FuncInfo>(m)) {}

    // 处理流程：两趟处理，mark 标记有用变量，sweep 删除无用指令
    void run() {
        int iter_count = 0;
        bool changed = false;
        func_info->run();
        do {
            LOG_DEBUG << "ADCE: before iter"
                      << "\n"
                      << m_->print(true);
            changed = false;
            ++iter_count;

            exit_if(iter_count > 10, ERROR_IN_DCE, "DCE: iteration not convergence");

            for (auto func : m_->get_functions()) {
                init(func.get());
                try_rewrite_ret(func);
                mark(func.get());
                changed |= sweep(func.get());
            }
            LOG_DEBUG << "ADCE: after iter"
                      << "\n"
                      << m_->print(true);
        } while (changed);
        LOG_INFO << "dead code pass erases " << ins_count << " instructions";
    }

  private:
    std::shared_ptr<FuncInfo> func_info;
    void try_rewrite_ret(std::shared_ptr<Function> func) {
        if (func->get_return_type()->is_void_type())
            return;
        if (func->get_name() == "main")
            return;

        bool flag = true;
        for (auto use : func->get_use_list())
            if (not use.val_->get_use_list().empty())
                flag = false;
        if (flag)
            for (auto bb : func->get_basic_blocks())
                if (auto retinst = std::dynamic_pointer_cast<ReturnInst>(bb->get_terminator())) {
                    retinst->remove_use_of_ops();
                    if (func->get_return_type()->is_integer_type())
                        retinst->set_operand(0, User::operand_t(ConstantInt::get(0, m_)->shared_from_this()));
                    else {
                        exit_if(func->get_return_type()->is_float_type() == false,
                                ERROR_IN_DCE,
                                "DCE: ret type is not void/int/float");
                        retinst->set_operand(0, User::operand_t(ConstantFP::get(0.0, m_)->shared_from_this()));
                    }
                }
    }
    // 初始化，主要是 RDF 相关计算的处理（因为要用到支配树）
    void init(Function *func) {
        postdom = std::make_unique<PostDominators>(m_);
        postdom->run();
        work_list.clear();
        markedbb.clear();
        markedins.clear();
    }
    void mark(Function *func) {
        for (auto bb : func->get_basic_blocks()) {
            for (auto ins : bb->get_instructions()) {
                if (is_critical(ins)) {
                    work_list.push_back(ins);
                }
            }
        }

        while (work_list.empty() == false) {
            auto now = work_list.front();
            work_list.pop_front();

            // assert(markedins[now] == true && "ADCE: a inst in worklist but not marked");
            mark(now);
        }
    }
    void mark(std::shared_ptr<Instruction> ins) {
        LOG_DEBUG << "ADCE: mark inst " << ins->print();
        if (markedins[ins])
            return;
        markedins[ins] = true;
        for (auto op : ins->get_operands()) {
            auto ptr = op.get()->shared_from_this();
            if (std::shared_ptr<Instruction> def = std::dynamic_pointer_cast<Instruction>(ptr)) {
                if (markedins[def])
                    continue;
                if (def->get_function() != ins->get_function())
                    continue;
                work_list.push_back(def);
                LOG_DEBUG << "ADCE: " << ins->print() << " marks " << def->print();
            }
            if (ins->is_phi())
                if (auto bb = std::dynamic_pointer_cast<BasicBlock>(ptr)) {
                    work_list.push_back(bb->get_terminator());
                    LOG_DEBUG << "ADCE: " << ins->print() << " marks " << bb->get_name();
                }
        }
        mark(ins->get_parent_shared());
    }
    void mark(std::shared_ptr<BasicBlock> bb) {
        if (is_active_bb(bb))
            return;
        markedbb.insert(bb);
        for (auto rdf : postdom->get_dom_frontier(bb.get())) {
            auto brinst = std::dynamic_pointer_cast<BranchInst>(rdf->get_terminator());
            if (brinst == nullptr)
                continue;
            if (markedins[brinst])
                continue;
            LOG_DEBUG << "ADCE: " << bb->get_name() << " marks " << brinst->print();
            work_list.push_back(brinst);
        }
    }
    bool is_instruction(std::shared_ptr<Value> val) { return std::dynamic_pointer_cast<Instruction>(val) != nullptr; }
    bool sweep(Function *func) {
        int ins_sweeped_this_time = 0;
        for (auto bb : func->get_basic_blocks()) {
            for (auto it = bb->get_instructions().begin(); it != bb->get_instructions().end();) {
                LOG_DEBUG << "ADCE: sweeping, inst " << (*it)->print() << " is alive? " << markedins[*it];
                if (markedins[*it] or (*it)->isTerminator()) {
                    ++it;
                    continue;
                }
                auto tmp = *it;
                it = bb->get_instructions().erase(it);
                tmp->remove_use_of_ops();
                ++ins_sweeped_this_time;
            }
            if (auto brinst = std::dynamic_pointer_cast<BranchInst>(bb->get_terminator())) {
                if (markedins[brinst])
                    continue;
                auto new_target = find_succ_live_block(bb);

                brinst->remove_use_of_ops();
                brinst->erase_from_parent();
                for (auto succ : bb->get_succ_basic_blocks())
                    succ->remove_pre_basic_block(bb.get());
                bb->get_succ_basic_blocks().clear();

                BranchInst::create_br(new_target, bb.get());
            }
        }
        for (auto bb : func->get_basic_blocks()) {
            LOG_DEBUG << "ADCE: bb " << bb->get_name() << ", active? " << is_active_bb(bb);
            if (is_active_bb(bb))
                continue;
            assert(markedins[bb->get_terminator()] == false && "inactive bb's term ins should not be marked");
        }
        ins_count += ins_sweeped_this_time;
        return ins_sweeped_this_time != 0;
    }
    std::shared_ptr<BasicBlock> find_succ_live_block(std::shared_ptr<BasicBlock> bb) {
        int iter_count = 0;
        bb = std::dynamic_pointer_cast<BasicBlock>(postdom->get_idom(bb.get())->shared_from_this());
        while (not is_active_bb(bb)) {
            assert(bb != nullptr and "meet nullptr in find succ live block");
            assert(++iter_count < 1000 and "endless finding succ live block");

            LOG_DEBUG << "ADCE: find idom of " << bb->get_name();
            bb = std::dynamic_pointer_cast<BasicBlock>(postdom->get_idom(bb.get())->shared_from_this());
        }
        return bb;
    }
    bool is_critical(std::shared_ptr<Instruction> ins) {
        // 对纯函数的无用调用也可以在删除之列
        if (ins->is_call()) {
            auto call_inst = std::dynamic_pointer_cast<CallInst>(ins);
            auto callee = std::dynamic_pointer_cast<Function>(call_inst->get_callee()->shared_from_this());
            if (func_info->is_pure_function(callee))
                return false;
            return true;
        }
        if (ins->is_ret())
            return true;
        if (ins->is_store())
            return true;
        return false;
    }
    bool is_active_bb(std::shared_ptr<BasicBlock> bb) { return markedbb.find(bb) != markedbb.end(); }
    bool is_branch(std::shared_ptr<Instruction> ins) { return ins->is_br(); }
    // 计算出 bb 的反向支配边界，用于标记有用指令
    std::unordered_set<BasicBlock *> RDF(BasicBlock *bb) { return std::unordered_set<BasicBlock *>{}; }

    // 用以衡量死代码消除的性能
    int ins_count{0};
    std::deque<std::shared_ptr<Instruction>> work_list{};
    std::unordered_map<std::shared_ptr<Instruction>, bool> markedins{};
    std::unordered_set<std::shared_ptr<BasicBlock>> markedbb{};
    std::unique_ptr<PostDominators> postdom;
};

/**
 * TODO: 完成四种前置的控制流简化，提升消除效果
 * TODO: 完成更加激进的死代码消除，消除无用的 store 指令
 * 常量传播：参见 https://www.zhihu.com/question/55976094/answer/147302764
 * 死代码消除：参见 https://www.clear.rice.edu/comp512/Lectures/10Dead-Clean-SCCP.pdf
 * 具体原理参见 编译器设计 p403
 **/
class DeadCode : public Pass {
  public:
    DeadCode(Module *m) : Pass(m), func_info(std::make_shared<FuncInfo>(m)) {}

    // 处理流程：两趟处理，mark 标记有用变量，sweep 删除无用指令
    void run() {
        int iter_count = 0;
        bool changed = false;
        func_info->run();
        do {
            changed = false;
            ++iter_count;

            exit_if(iter_count > 1000, ERROR_IN_DCE, "DCE: iteration not convergence");

            for (auto func : m_->get_functions()) {
                try_rewrite_ret(func);
                init(func.get());
                mark(func.get());
                changed |= sweep(func.get());
            }
        } while (changed);
        LOG_INFO << "dead code pass erases " << ins_count << " instructions";
    }

  private:
    std::shared_ptr<FuncInfo> func_info;
    void try_rewrite_ret(std::shared_ptr<Function> func) {
        if (func->get_return_type()->is_void_type())
            return;
        if (func->get_name() == "main")
            return;

        bool flag = true;
        for (auto use : func->get_use_list())
            if (not use.val_->get_use_list().empty())
                flag = false;
        if (flag)
            for (auto bb : func->get_basic_blocks())
                if (auto retinst = std::dynamic_pointer_cast<ReturnInst>(bb->get_terminator())) {
                    retinst->remove_use_of_ops();
                    if (func->get_return_type()->is_integer_type())
                        retinst->set_operand(0, User::operand_t(ConstantInt::get(0, m_)->shared_from_this()));
                    else {
                        exit_if(func->get_return_type()->is_float_type() == false,
                                ERROR_IN_DCE,
                                "DCE: ret type is not void/int/float");
                        retinst->set_operand(0, User::operand_t(ConstantFP::get(0.0, m_)->shared_from_this()));
                    }
                }
    }
    // 初始化，主要是 RDF 相关计算的处理（因为要用到支配树）
    void init(Function *func) {}
    void mark(Function *func) {
        work_list.clear();
        marked.clear();

        for (auto bb : func->get_basic_blocks()) {
            for (auto ins : bb->get_instructions()) {
                if (is_critical(ins)) {
                    marked[ins] = true;
                    work_list.push_back(ins);
                }
            }
        }

        while (work_list.empty() == false) {
            auto now = work_list.front();
            work_list.pop_front();

            mark(now);
        }
    }
    void mark(std::shared_ptr<Instruction> ins) {
        for (auto op : ins->get_operands()) {
            auto ptr = op.get()->shared_from_this();
            std::shared_ptr<Instruction> def = std::dynamic_pointer_cast<Instruction>(ptr);
            if (def == nullptr)
                continue;
            if (marked[def])
                continue;
            if (def->get_function() != ins->get_function())
                continue;
            marked[def] = true;
            work_list.push_back(def);
        }
    }
    bool is_instruction(std::shared_ptr<Value> val) { return std::dynamic_pointer_cast<Instruction>(val) != nullptr; }
    bool sweep(Function *func) {
        int ins_sweeped_this_time = 0;
        for (auto bb : func->get_basic_blocks()) {
            std::list<std::shared_ptr<Instruction>> wait_del{};
            for (auto it = bb->get_instructions().begin(); it != bb->get_instructions().end();) {
                if (marked[*it]) {
                    ++it;
                    continue;
                } else {
                    auto tmp = *it;
                    it = bb->get_instructions().erase(it);
                    tmp->remove_use_of_ops();
                    ++ins_sweeped_this_time;
                }
            }
        }
        ins_count += ins_sweeped_this_time;
        return ins_sweeped_this_time != 0;
    }
    bool is_critical(std::shared_ptr<Instruction> ins) {
        // 对纯函数的无用调用也可以在删除之列
        if (ins->is_call()) {
            auto call_inst = std::dynamic_pointer_cast<CallInst>(ins);
            auto callee = std::dynamic_pointer_cast<Function>(call_inst->get_callee()->shared_from_this());
            if (func_info->is_pure_function(callee))
                return false;
            return true;
        }
        if (ins->is_br() || ins->is_ret() || ins->is_switch())
            return true;
        if (ins->is_store())
            return true;
        return false;
    }
    bool is_branch(std::shared_ptr<Instruction> ins) { return ins->is_br(); }
    // 计算出 bb 的反向支配边界，用于标记有用指令
    std::unordered_set<BasicBlock *> RDF(BasicBlock *bb) { return std::unordered_set<BasicBlock *>{}; }

    // 用以衡量死代码消除的性能
    int ins_count{0};
    std::deque<std::shared_ptr<Instruction>> work_list{};
    std::unordered_map<std::shared_ptr<Instruction>, bool> marked{};
};
