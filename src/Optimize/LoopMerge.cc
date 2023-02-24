#include "LoopMerge.hh"

#include "logging.hpp"
#include "utils.hh"

#include <map>

using std::map;

void LoopMerge::run() {
    LOG_INFO << "run LoopMerge";
    _loop_info.run();

    map<Loop *, vector<Loop *>> parent_to_children;

    for (auto &loop : _loop_info.get_loops()) {
        auto pre_block = loop.get_preheader();
        auto base_block = loop.get_base();
        auto compare_block = loop.get_latch();
        auto exit_block = loop.get_exit();

        auto func = pre_block->get_parent();
        LOG_DEBUG << "func: " << func->get_name();
        LOG_DEBUG << "child: " << pre_block->get_name();
        if (func->get_name() == "spmv") {
            auto parent = loop.get_parent_loop();
            LOG_DEBUG << "parent: " << parent;
            if (parent) {
                auto parent_pre_block = parent->get_preheader();
                LOG_DEBUG << "parent: " << parent_pre_block->get_name();
                parent_to_children[parent].push_back(&loop);
            }
        }
    }

    for (auto [parent, children] : parent_to_children) {
        auto parent_pre_block = parent->get_preheader();
        LOG_DEBUG << "Parent: " << parent_pre_block->get_name();
        for (auto child : children) {
            auto child_pre_block = child->get_preheader();
            LOG_DEBUG << "Child: " << child_pre_block->get_name();
        }
    }

    for (auto [parent, children] : parent_to_children) {
        if (children.size() == 2) {
            auto loop1 = children[0];
            auto loop2 = children[1];

            auto temp_exit_block1 = loop1->get_exit();
            auto temp_after_block1 = std::dynamic_pointer_cast<BasicBlock>(
                temp_exit_block1->get_succ_basic_blocks().front()->shared_from_this());
            auto temp_target_after_block1 = std::dynamic_pointer_cast<BasicBlock>(
                temp_after_block1->get_succ_basic_blocks().front()->shared_from_this());

            auto temp_pre_block2 = loop2->get_preheader();
            auto temp_before_block2 = std::dynamic_pointer_cast<BasicBlock>(
                temp_pre_block2->get_pre_basic_blocks().front()->shared_from_this());

            if (temp_target_after_block1 != temp_before_block2) {
                loop1 = children[1];
                loop2 = children[0];
            }

            auto pre_block1 = loop1->get_preheader();
            auto base_block1 = loop1->get_base();
            auto compare_block1 = loop1->get_latch();
            auto exit_block1 = loop1->get_exit();
            auto after_block1 =
                std::dynamic_pointer_cast<BasicBlock>(exit_block1->get_succ_basic_blocks().front()->shared_from_this());
            auto before_block1 =
                std::dynamic_pointer_cast<BasicBlock>(pre_block1->get_pre_basic_blocks().front()->shared_from_this());
            auto target_after_block1 = std::dynamic_pointer_cast<BasicBlock>(
                after_block1->get_succ_basic_blocks().front()->shared_from_this());

            auto pre_block2 = loop2->get_preheader();
            auto base_block2 = loop2->get_base();
            auto compare_block2 = loop2->get_latch();
            auto exit_block2 = loop2->get_exit();
            auto after_block2 =
                std::dynamic_pointer_cast<BasicBlock>(exit_block2->get_succ_basic_blocks().front()->shared_from_this());
            auto before_block2 =
                std::dynamic_pointer_cast<BasicBlock>(pre_block2->get_pre_basic_blocks().front()->shared_from_this());
            auto target_after_block2 = std::dynamic_pointer_cast<BasicBlock>(
                after_block2->get_succ_basic_blocks().front()->shared_from_this());

            assert(target_after_block1 == before_block2);

            // all_base_block2_inst
            auto self_add_inst2 = base_block2->get_instructions().end().operator--().operator--();
            base_block2->delete_instr(self_add_inst2);

            vector<Instruction *> all_base_block2_inst;
            for (auto inst : base_block2->get_instructions())
                if (not inst->is_phi() and not inst->is_br())
                    all_base_block2_inst.push_back(inst.get());

            auto self_add_inst1 = base_block1->get_instructions().end().operator--().operator--()->get();

            for (auto inst : all_base_block2_inst) {
                base_block1->insert_before(std::dynamic_pointer_cast<Instruction>(self_add_inst1->shared_from_this()),
                                           std::dynamic_pointer_cast<Instruction>(inst->shared_from_this()));
                inst->set_parent(base_block1.get());
                LOG_DEBUG << "inst: " << inst->print() << " parent: " << inst->get_parent()->get_name();
            }

            for (auto inst_it = base_block2->get_instructions().begin();
                 inst_it != base_block2->get_instructions().end();) {
                auto inst = inst_it->get();
                if (not inst->is_phi() and not inst->is_br()) {
                    inst_it = base_block2->get_instructions().erase(inst_it);
                } else {
                    inst_it++;
                }
            }
            // after_block1 跳转到 target_after_block2
            after_block1->delete_instr(after_block1->get_instructions().end().operator--());
            after_block1->remove_succ_basic_block(target_after_block1.get());
            target_after_block1->remove_pre_basic_block(after_block1.get());

            auto after_block1_jump_inst = BranchInst::create_br(target_after_block2, after_block1.get());

            auto func = pre_block1->get_parent();

            // replace after_block2 with after_block1 in phi
            after_block2->replace_all_use_with(after_block1);

            // replace phi_inst in after_block2
            for (auto inst : after_block2->get_instructions()) {
                if (inst->is_phi()) {
                    auto phi_inst = std::dynamic_pointer_cast<PhiInst>(inst);
                    int pair_num = phi_inst->get_num_operand() / 2;
                    for (int i = 0; i < pair_num; i++) {
                        auto value = phi_inst->get_operand(i * 2).get();
                        auto bb = phi_inst->get_operand(i * 2 + 1).get();
                        if (bb == before_block2.get())
                            phi_inst->replace_all_use_with(value->shared_from_this());
                    }
                }
            }
            vector<Instruction *> all_after_block2_inst;
            for (auto inst : after_block2->get_instructions())
                if (not inst->is_phi() and not inst->is_br())
                    all_after_block2_inst.push_back(inst.get());

            for (auto inst : all_after_block2_inst) {
                after_block1->insert_before(
                    std::dynamic_pointer_cast<Instruction>(after_block1_jump_inst->shared_from_this()),
                    std::dynamic_pointer_cast<Instruction>(inst->shared_from_this()));
                LOG_DEBUG << "inst: " << inst->print();
                inst->set_parent(after_block1.get());
            }
            for (auto inst_it = after_block2->get_instructions().begin();
                 inst_it != after_block2->get_instructions().end();) {
                auto inst = inst_it->get();
                if (not inst->is_phi() and not inst->is_br()) {
                    inst_it = after_block2->get_instructions().erase(inst_it);
                } else {
                    inst_it++;
                }
            }

            auto ind_var1 = loop1->get_ind_vars().begin()->get()->step_inst;
            auto phi_val1 = ind_var1->get_operand(0);

            auto ind_var2 = loop2->get_ind_vars().begin()->get()->step_inst;
            auto phi_val2 = ind_var2->get_operand(0);

            phi_val2->replace_all_use_with(phi_val1->shared_from_this());

            LOG_DEBUG << "after_block1: " << after_block1->get_name();
            LOG_DEBUG << "after_block2: " << after_block2->get_name();

            LOG_DEBUG << "delete before_block2: " << before_block2->get_name();
            func->remove_unreachable_basic_block(pre_block2);
            func->remove_unreachable_basic_block(base_block2);
            func->remove_unreachable_basic_block(compare_block2);
            func->remove_unreachable_basic_block(after_block2);
            func->remove_unreachable_basic_block(before_block2);
            func->remove_unreachable_basic_block(exit_block2);
        }
    }
}
