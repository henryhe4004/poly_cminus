#pragma once
#include "Dominators.hh"
#include "FuncInfo.hh"
#include "Function.h"
#include "Pass.hh"
#include "logging.hpp"

#include <deque>
#include <unordered_map>
#include <unordered_set>

/**
 * @brief 提取基本块中的公共子表达式
 * 原理参见 高级编译器设计与实现-局部公共子表达式删除
 * UPD: 同时实现了超级块的公共子表达式删除和支配树的子表达式删除
 * UPD: 在 hash 时没有考虑移位指令，因此不能在 lir pass 后调用
 */
class LocalCommonExpression : public Pass {
  private:
    std::unique_ptr<FuncInfo> funcinfo;

  public:
    LocalCommonExpression(Module *m) : Pass(m) {}

    void run() {
        funcinfo = std::make_unique<FuncInfo>(m_);
        funcinfo->run();

        BBLCE();
        EBBLCE();
        DVNT();

        LOG_INFO << "local common expression process " << ins_count << " instructions";
    }

  private:
    void BBLCE() {
        auto func_list = m_->get_functions();
        for (auto func : func_list) {
            if (func->get_num_basic_blocks() == 0)
                continue;
            for (auto bb : func->get_basic_blocks()) {
                process_bb(bb);
            }
        }
    }

    using llu = unsigned long long;
    static constexpr llu mod = 1e9 + 7;
    // DONE: 对具有交换律的二元运算特殊处理
    // DONE: 考虑加入对 GEP 指令的特殊处理？
    // DONE: 经过测试发现，以下代码中的 1 具有不同的地址，较为奇怪？
    // DONE: 在为常数值分配同样的指针后这个问题已经修复
    /** 表现异常的代码
        int fun() {
            int a = 1, b = 1;
            int c = a + 1;
            int d = a + 1;
            int e = c + d;
            return e;
        }
     */
    class myhash {
      public:
        std::size_t operator()(std::shared_ptr<Instruction> now) const { return now->get_hash(); };
    };

    class myequal {
      public:
        bool operator()(const std::shared_ptr<Instruction> &lhs, const std::shared_ptr<Instruction> &rhs) const {
            return lhs->equal(rhs);
        };
    };

    // AEB: 可用表达式的集合
    using expset = std::unordered_set<std::shared_ptr<Instruction>, myhash, myequal>;
    expset aeb{};
    void process_bb(std::shared_ptr<BasicBlock> bb) {
        aeb.clear();
        auto inses = bb->get_instructions();
        for (auto it = inses.begin(); it != inses.end(); ++it) {
            auto inst = *it;
            if (can_be_common_expression(inst)) {
                if (aeb.find(inst) != aeb.end()) {
                    auto pre_ins = *aeb.find(inst);
                    // 清理工作交给死代码删除
                    inst->replace_all_use_with(pre_ins);
                    ++ins_count;
                } else {
                    aeb.insert(inst);
                }
            }
        }
    }

    bool can_be_common_expression(std::shared_ptr<Instruction> inst) {
        if (inst->is_hash_able())
            return true;
        if (inst->is_call()) {
            auto callee = std::dynamic_pointer_cast<Function>(inst->get_operand(0)->shared_from_this());
            if (funcinfo->is_pure_function(callee))
                return true;
        }
        return false;
    }

    std::shared_ptr<Instruction> find_in_aeb_chain(std::shared_ptr<std::deque<expset>> aebs,
                                                   std::shared_ptr<Instruction> inst) {
        for (auto exps : *aebs) {
            auto res = exps.find(inst);
            if (res != exps.end())
                return *res;
        }
        return nullptr;
    }
    void ins_to_aeb_chain(std::shared_ptr<std::deque<expset>> aebs, std::shared_ptr<Instruction> inst) {
        aebs->back().insert(inst);
    }
    void LVN(std::shared_ptr<BasicBlock> bb, std::shared_ptr<std::deque<expset>> aebs) {
        for (auto inst : bb->get_instructions()) {
            if (can_be_common_expression(inst)) {
                if (auto prevdef = find_in_aeb_chain(aebs, inst)) {
                    inst->replace_all_use_with(prevdef);
                    ++ins_count;
                } else
                    ins_to_aeb_chain(aebs, inst);
            }
        }
    }

    std::unique_ptr<std::deque<std::shared_ptr<BasicBlock>>> worklist;
    std::unique_ptr<std::unordered_set<std::shared_ptr<BasicBlock>>> processedbb;
    void EBBLCE() {
        worklist = std::make_unique<std::deque<std::shared_ptr<BasicBlock>>>();
        for (auto func : m_->get_functions()) {
            if (func->is_declaration())
                continue;

            worklist->push_back(func->get_entry_block());

            auto aebs = std::make_shared<std::deque<expset>>();
            processedbb = std::make_unique<std::unordered_set<std::shared_ptr<BasicBlock>>>();
            while (worklist->empty() == false) {
                auto bb = worklist->front();
                worklist->pop_front();

                SVN(bb, aebs);
            }
        }
    }

    void SVN(std::shared_ptr<BasicBlock> bb, std::shared_ptr<std::deque<expset>> aebs) {
        if (processedbb->find(bb) != processedbb->end())
            return;
        processedbb->insert(bb);
        aebs->push_back(expset());
        LVN(bb, aebs);
        for (auto succ : bb->get_succ_basic_blocks()) {
            if (succ->get_pre_basic_blocks().size() == 1)
                SVN(std::dynamic_pointer_cast<BasicBlock>(succ->shared_from_this()), aebs);
            else if (processedbb->find(std::dynamic_pointer_cast<BasicBlock>(succ->shared_from_this())) ==
                     processedbb->end())
                worklist->push_back(std::dynamic_pointer_cast<BasicBlock>(succ->shared_from_this()));
        }
        aebs->pop_back();
    }

    std::unique_ptr<Dominators> dom;
    void DVNT() {
        dom = std::make_unique<Dominators>(m_);
        dom->run();
        for (auto func : m_->get_functions()) {
            if (func->is_declaration())
                continue;

            auto aebs = std::make_shared<std::deque<expset>>();
            DVNT(func->get_entry_block(), aebs);
        }
    }

    void DVNT(std::shared_ptr<BasicBlock> bb, std::shared_ptr<std::deque<expset>> aebs) {
        aebs->push_back(expset());

        // phi 函数：1. 只考虑块内的公共子表达式 2. 处理单一的 phi 表达式
        // 但这两步都会在别的 pass 中处理。1 会在之前的公共子表达式中处理，2 会在控制流化简处理特殊 phi 函数时处理

        for (auto inst : bb->get_instructions()) {
            if (inst->is_phi())
                continue;
            if (can_be_common_expression(inst)) {
                if (auto prevdef = find_in_aeb_chain(aebs, inst)) {
                    LOG_DEBUG << "CommonExp: replace " << inst->print() << " with " << prevdef->print();
                    inst->replace_all_use_with(prevdef);
                    ++ins_count;
                } else
                    ins_to_aeb_chain(aebs, inst);
            }
        }

        for (auto succ : dom->get_dom_tree_succ_blocks(bb.get()))
            DVNT(std::dynamic_pointer_cast<BasicBlock>(succ->shared_from_this()), aebs);

        aebs->pop_back();
    }

    int ins_count{0};
};
