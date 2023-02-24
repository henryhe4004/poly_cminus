#ifndef _SIMPLIFY_CFG_HH_
#define _SIMPLIFY_CFG_HH_

#include "Pass.hh"
#include "errorcode.hh"
#include "logging.hpp"
#include "utils.hh"

#include <algorithm>
#include <deque>

class SimplifyCFG : public Pass {
  public:
    SimplifyCFG(Module *m) : Pass(m) {}

    void run() {
        for (auto func : m_->get_functions()) {
            if (func->get_num_basic_blocks() == 0)
                continue;
            LOG_INFO << "SimpCFG: " << func->get_name() << " has " << func->get_num_basic_blocks() << " bbs";
            rewrite_true_br(func);
            fix_wrong_phi(func);

            int iter_count = 0;
            bool changed = false;
            do {
                ++iter_count;
                assert(iter_count < 1000 and "SimpCFG: iter not convergence");

                changed = false;
                changed |= remove_unreachable_blocks(func);

                std::deque<std::shared_ptr<BasicBlock>> pro;
                std::unordered_set<std::shared_ptr<BasicBlock>> visited;
                get_pro(func->get_entry_block(), pro, visited);
                changed |= simplify_once(func, pro);
                elim_single_phi(func);
            } while (changed);
        }
    }

  private:
    void fix_wrong_phi(std::shared_ptr<Function> func) {
        for (auto bb : func->get_basic_blocks()) {
            for (auto inst : bb->get_instructions()) {
                if (not inst->is_phi())
                    break;
                auto phiinst = std::dynamic_pointer_cast<PhiInst>(inst);
                for (int i = 0; i < phiinst->get_num_operand(); i += 2) {
                    auto pred = dynamic_cast<BasicBlock *>(phiinst->get_operand(i + 1).get());
                    auto predbbs = bb->get_pre_basic_blocks();
                    if (std::find(predbbs.begin(), predbbs.end(), pred) == predbbs.end()) {
                        phiinst->remove_operands(i, i + 1);
                        i -= 2;
                    }
                }
            }
        }
    }

    void elim_single_phi(std::shared_ptr<Function> func) {
        for (auto bb : func->get_basic_blocks()) {
            if (bb->get_pre_basic_blocks().size() != 1)
                continue;
            std::deque<std::shared_ptr<Instruction>> wait_del;
            for (auto inst : bb->get_instructions()) {
                if (not inst->is_phi())
                    break;
                if (inst->get_num_operand() == 0) {
                    LOG_WARNING << "A phi inst with zero operand " << inst->print();
                    continue;
                }
                // TODO: 这样的 phi 指令不应该出现，需要确定出现的原因
                if (inst->get_operand(1)->shared_from_this() !=
                    (*(bb->get_pre_basic_blocks().begin()))->shared_from_this())
                    continue;
                assert(inst->get_operand(0)->shared_from_this() != inst && "SimpCFG: illegal undef phi inst");
                inst->replace_all_use_with(inst->get_operand(0)->shared_from_this());
                wait_del.push_back(inst);
            }
            for (auto inst : wait_del)
                inst->erase_from_parent();
        }
    }

    void get_pro(std::shared_ptr<BasicBlock> bb,
                 std::deque<std::shared_ptr<BasicBlock>> &pro,
                 std::unordered_set<std::shared_ptr<BasicBlock>> &visited) {
        LOG_DEBUG << "SimpCFG: visit " << bb->get_name();
        if (visited.find(bb) != visited.end())
            return;
        visited.insert(bb);

        if (auto brinst = std::dynamic_pointer_cast<BranchInst>(bb->get_terminator())) {
            if (brinst->is_cond_br()) {
                auto iftrue = std::dynamic_pointer_cast<BasicBlock>(brinst->get_operand(1)->shared_from_this());
                auto iffalse = std::dynamic_pointer_cast<BasicBlock>(brinst->get_operand(2)->shared_from_this());
                get_pro(iftrue, pro, visited);
                get_pro(iffalse, pro, visited);
            } else {
                auto jump = std::dynamic_pointer_cast<BasicBlock>(brinst->get_operand(0)->shared_from_this());
                get_pro(jump, pro, visited);
            }
        }
        pro.push_back(bb);
    }
    bool remove_unreachable_blocks(std::shared_ptr<Function> func) {
        std::unordered_set<std::shared_ptr<BasicBlock>> wait_del;
        for (auto bb : func->get_basic_blocks())
            wait_del.insert(bb);
        LOG_INFO << "SimpCFG: before remove unreachable blocks has " << wait_del.size() << " bbs";
        visit_bb(func->get_entry_block(), wait_del);
        LOG_INFO << "SimpCFG: after remove unreachable blocks has " << wait_del.size() << " bbs";

        for (auto bb : wait_del) {
            if (bb == func->get_entry_block())
                continue;
            if (isa<ReturnInst>(bb->get_terminator())) {
                // 死循环
                LOG_ERROR << "SimpCFG: inf loop found";
                exit(ERROR_IN_SIMP_CFG);
            }
            remove_bb_in_phis(bb);
            func->remove_basic_block(bb);
        }
        if (wait_del.size())
            return true;
        return false;
    }
    void remove_bb_in_phis(std::shared_ptr<BasicBlock> bb) {
        for (auto succ : bb->get_succ_basic_blocks()) {
            std::deque<std::shared_ptr<Instruction>> wait_del;
            for (auto inst : succ->get_instructions()) {
                if (not inst->is_phi())
                    break;
                auto phiinst = std::dynamic_pointer_cast<PhiInst>(inst);
                phiinst->remove_bb(bb);
                // assert(phiinst->get_num_operand() != 0 && "SimpCFG: a phi inst with zero operand");
                if (phiinst->get_num_operand() <= 2)
                    continue;
                if (auto val = has_constant_value(phiinst)) {
                    LOG_DEBUG << "SimpCFG: replace " << phiinst->print() << " with " << val->print();
                    phiinst->replace_all_use_with(val);
                    wait_del.push_back(phiinst);
                }
            }
            for (auto inst : wait_del)
                inst->erase_from_parent();
        }
    }
    std::shared_ptr<Value> has_constant_value(std::shared_ptr<PhiInst> phiinst) {
        auto constant_val = phiinst->get_operand(0)->shared_from_this();
        for (int i = 2; i < phiinst->get_num_operand(); i += 2) {
            auto val = phiinst->get_operand(i)->shared_from_this();
            if (val != constant_val and val != phiinst) {
                if (constant_val != phiinst)
                    return nullptr;
                constant_val = phiinst->get_operand(i)->shared_from_this();
            }
        }
        if (constant_val == phiinst)
            return nullptr;
        return constant_val;
    }
    void try_replace_single_phi(std::shared_ptr<PhiInst> phiinst) {
        if (phiinst->get_num_operand() != 2)
            return;
        if (phiinst == phiinst->get_operand(0)->shared_from_this())
            return;
        phiinst->replace_all_use_with(phiinst->get_operand(0)->shared_from_this());
        phiinst->erase_from_parent();
    }
    void visit_bb(std::shared_ptr<BasicBlock> bb, std::unordered_set<std::shared_ptr<BasicBlock>> &wait_del) {
        if (wait_del.find(bb) == wait_del.end())
            return;
        wait_del.erase(wait_del.find(bb));
        if (isa<ReturnInst>(bb->get_terminator()))
            return;
        if (isa<BranchInst>(bb->get_terminator())) {
            auto brinst = std::dynamic_pointer_cast<BranchInst>(bb->get_terminator());
            if (brinst->is_cond_br()) {
                auto iftrue = std::dynamic_pointer_cast<BasicBlock>(brinst->get_operand(1)->shared_from_this());
                auto iffalse = std::dynamic_pointer_cast<BasicBlock>(brinst->get_operand(2)->shared_from_this());
                visit_bb(iftrue, wait_del);
                visit_bb(iffalse, wait_del);
            } else {
                auto jump = std::dynamic_pointer_cast<BasicBlock>(brinst->get_operand(0)->shared_from_this());
                visit_bb(jump, wait_del);
            }
        } else if (isa<SwitchInst>(bb->get_terminator())) {
            auto sw = std::dynamic_pointer_cast<SwitchInst>(bb->get_terminator());
            for (size_t i = 1; i < sw->get_num_operand(); i += 2) {
                auto target = std::dynamic_pointer_cast<BasicBlock>(sw->get_operand(i)->shared_from_this());
                visit_bb(target, wait_del);
            }
        }
    }
    bool simplify_once(std::shared_ptr<Function> func, const std::deque<std::shared_ptr<BasicBlock>> &pro) {
        bool changed = false;
        for (auto bb : pro) {
            if (isa<BranchInst>(bb->get_terminator()) and bb->get_terminator()->get_num_operand() == 3)
                changed |= merge_double_br(bb);
            if (isa<BranchInst>(bb->get_terminator())) {
                changed |= try_replace_empty_jump(bb);
                elim_single_phi(func);
                changed |= try_combine(bb);
                // changed |= rewrite_jump(bb);
            }
            // 这个优化貌似没有太多必要
            // if (isa<ReturnInst>(bb->get_terminator())) {
            //     changed |= try_replace_empty_jump(bb);
            // }
        }
        return changed;
    }
    bool try_replace_empty_jump(std::shared_ptr<BasicBlock> bb) {
        if (*bb->get_instructions().begin() != bb->get_terminator())
            return false;
        if (bb->get_pre_basic_blocks().size() == 0)
            return false;
        if (isa<BranchInst>(bb->get_terminator())) {
            auto brinst = std::dynamic_pointer_cast<BranchInst>(bb->get_terminator());
            // TODO: 条件跳转可以合并
            if (brinst->is_cond_br()) {
                return false;
            } else {
                auto succ = brinst->get_target();
                for (auto pred : bb->get_pre_basic_blocks()) {
                    auto brinst = std::dynamic_pointer_cast<BranchInst>(pred->get_terminator());
                    if (brinst->is_cond_br()) {
                        auto iftrue = brinst->get_true_succ();
                        auto iffalse = brinst->get_false_succ();
                        if (iftrue == succ or iffalse == succ)
                            return false;
                    }
                }
                // 对后继进行修改
                for (auto inst : succ->get_instructions()) {
                    if (not inst->is_phi())
                        break;
                    auto phiinst = std::dynamic_pointer_cast<PhiInst>(inst);
                    auto val = phiinst->input_of(bb.get());
                    if (val == nullptr)
                        continue;
                    phiinst->remove_bb(bb);
                    for (auto pred : bb->get_pre_basic_blocks()) {
                        phiinst->add_phi_pair_operand(val,
                                                      std::dynamic_pointer_cast<BasicBlock>(pred->shared_from_this()));
                    }
                }
                // 对前驱进行修改
                bb->replace_all_use_with(succ);
                succ->remove_pre_basic_block(bb.get());
                for (auto pred : bb->get_pre_basic_blocks()) {
                    pred->remove_succ_basic_block(bb.get());
                    pred->remove_succ_basic_block(succ.get());
                    succ->remove_pre_basic_block(pred);

                    succ->add_pre_basic_block(pred);
                    pred->add_succ_basic_block(succ.get());
                }
                return true;
            }
        } else {
            for (auto pred : bb->get_pre_basic_blocks()) {
                pred->remove_succ_basic_block(bb.get());
                pred->delete_instr(pred->get_terminator());
                // pred->insert_before(pred->get_instructions().end(), bb->get_terminator());
            }
            return true;
        }
    }
    bool try_combine(std::shared_ptr<BasicBlock> bb) {
        LOG_DEBUG << "SimpCFG: bug fix " << bb->get_name() << " " << bb->get_pre_basic_blocks().size();
        if (bb->get_pre_basic_blocks().size() != 1)
            return false;
        auto pred = std::dynamic_pointer_cast<BasicBlock>((*bb->get_pre_basic_blocks().begin())->shared_from_this());
        auto predbr = std::dynamic_pointer_cast<BranchInst>(pred->get_terminator());
        LOG_DEBUG << "SimpCFG: bug fix " << bb->get_name() << " " << predbr->print();
        if (predbr->get_num_operand() != 1)
            return false;
        // bb 可能已经被重写但是还没被删除
        if (predbr->get_target() != bb)
            return false;
        pred->delete_instr(pred->get_terminator());
        // 不需要调整 bb 的 phi 指令
        assert((*bb->get_instructions().begin())->is_phi() == false &&
               "try_combine: there should be no phi inst in this bb!");
        for (auto inst : bb->get_instructions()) {
            inst->set_parent(pred.get());
            pred->add_instruction(inst);
        }
        pred->get_succ_basic_blocks().clear();
        for (auto succ : bb->get_succ_basic_blocks()) {
            pred->add_succ_basic_block(succ);
            succ->remove_pre_basic_block(bb.get());
            succ->add_pre_basic_block(pred.get());
        }

        bb->get_pre_basic_blocks().clear();
        bb->get_succ_basic_blocks().clear();
        bb->replace_all_use_with(pred);
        // 因为是按照 pro 顺序调用的，因此可以直接移除
        bb->get_parent()->remove(bb);
        return true;
    }
    bool rewrite_jump(std::shared_ptr<BasicBlock> bb) { return false; }
    bool merge_double_br(std::shared_ptr<BasicBlock> bb) {
        auto brinst = std::dynamic_pointer_cast<BranchInst>(bb->get_terminator());
        auto iftrue = std::dynamic_pointer_cast<BasicBlock>(brinst->get_operand(1)->shared_from_this());
        auto iffalse = std::dynamic_pointer_cast<BasicBlock>(brinst->get_operand(2)->shared_from_this());

        if (iftrue == iffalse) {
            brinst->erase_from_parent();
            bb->remove_succ_basic_block(iftrue.get());
            iftrue->remove_pre_basic_block(bb.get());
            BranchInst::create_br(iftrue, bb.get());
            return true;
        }
        return false;
    }
    void rewrite_true_br(std::shared_ptr<Function> f) {
        for (auto bb : f->get_basic_blocks()) {
            auto term = std::dynamic_pointer_cast<BranchInst>(bb->get_terminator());
            if (term and utils::is_const_int(term->get_operand(0).get(), 1)) {
                for (auto succ : bb->get_succ_basic_blocks())
                    succ->remove_pre_basic_block(bb.get());
                bb->get_succ_basic_blocks().clear();
                auto target = std::dynamic_pointer_cast<BasicBlock>(term->get_operand(1)->shared_from_this());
                bb->delete_instr(bb->get_terminator_itr());
                BranchInst::create_br(target, bb.get());
            }
        }
    }
};

#endif