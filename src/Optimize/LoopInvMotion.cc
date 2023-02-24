#include "LoopInvMotion.hh"

#include "logging.hpp"

void LoopInvMotion::run() {
    LOG_INFO << "running loop-invarint code motion pass";
    _loops.run();
    _dom.run();
    func_info.run();

    std::vector<std::shared_ptr<bb_set_t>> order;
    std::vector<std::shared_ptr<bb_set_t>> make_order;
    for (auto loop : _loops) {
        if (_loops.get_parent_loop(loop) == nullptr) {
            // make_order.clear();
            make_order.push_back(loop);
            while (!make_order.empty()) {
                auto cur_loop = make_order.back();
                make_order.pop_back();
                order.push_back(cur_loop);
                for (auto sub : get_sub_loops(cur_loop)) {
                    make_order.push_back(sub);
                }
            }
        }
    }

    int finished = 0;
    LOG_DEBUG << order.size() << " loops to deal";
    // 处理每个循环，外提其中的不变式
    while (!order.empty()) {
        auto loop = order.back();
        order.pop_back();
        auto base = _loops.get_loop_base(loop);
        BasicBlock *loop_pre = nullptr, *loop_end = nullptr;
        for (auto pre : base->get_pre_basic_blocks()) {
            if (loop->find(pre) != loop->end())
                loop_end = pre;
            else
                loop_pre = pre;
        }
        if (!loop_pre) {
            continue;
        }
        auto inv_bb = loop_pre;
        auto pre_br = inv_bb->get_instructions().end();
        pre_br--;
        // inv_bb->delete_instr(pre_br);
        // auto inv_bb = BasicBlock::create(m_, "loopInv" + std::to_string(counter++), base->get_parent());
        // inv_bb->get_parent()->get_basic_blocks().pop_back();
        // inv_bb->add_pre_basic_block(loop_pre);
        // inv_bb->add_succ_basic_block(base);
        // base->get_pre_basic_blocks().remove(loop_pre);
        // base->add_pre_basic_block(inv_bb.get());
        // auto pre_br = loop_pre->get_terminator();
        // if (pre_br->get_num_operand() == 3) {
        //     // LOG_ERROR << "loop"
        //     for (int i = 1; i < 3; i++) {
        //         if (pre_br->get_operand(i).get() == base) {
        //             pre_br->set_operand(i, std::static_pointer_cast<Value>(inv_bb));
        //         }
        //     }
        // } else
        //     pre_br->set_operand(0, std::static_pointer_cast<Value>(inv_bb));
        // loop_pre->delete_instr(pre_br);

        // 找循环中的“变式”
        std::unordered_set<Value *> change_values;
        bool add_new = true;
        while (add_new) {
            add_new = false;
            for (auto bb : *loop) {
                for (auto instr : bb->get_instructions()) {
                    if (change_values.find(instr.get()) != change_values.end())
                        continue;
                    if (instr->is_load() || instr->is_phi()) {
                        change_values.insert(instr.get());
                        add_new = true;
                        continue;
                    } else if (instr->is_call()) {
                        if (!func_info.is_pure_function(
                                std::static_pointer_cast<Function>(instr->get_operand(0)->shared_from_this()))) {
                            change_values.insert(instr.get());
                            add_new = true;
                            continue;
                        }
                    }
                    for (auto operand : instr->get_operands()) {
                        // if (instr->get_name() == "op13") {
                        //     LOG_DEBUG << instr->get_name();
                        // }
                        if (change_values.find(operand.get()) != change_values.end()) {
                            change_values.insert(instr.get());
                            add_new = true;
                            break;
                        }
                    }
                }
            }
        }

        for (auto bb : *loop) {
            std::unordered_set<BasicBlock *> bb_doms;
            get_all_doms(bb, bb_doms);
            if (bb == loop_end || bb_doms.find(loop_end) != bb_doms.end()) {
                vector<shared_ptr<Instruction>> motion_list;
                for (auto instr = bb->get_instructions().begin(); instr != bb->get_instructions().end();) {
                    switch ((*instr)->get_instr_type()) {
                        // case Instruction::add:
                        // case Instruction::sub:
                        // case Instruction::mul:
                        // case Instruction::sdiv:
                        // case Instruction::srem:
                        // case Instruction::fadd:
                        // case Instruction::fsub:
                        // case Instruction::fmul:
                        // case Instruction::fdiv:
                        // case Instruction::cmp:
                        // case Instruction::fcmp:
                        // case Instruction::zext:
                        // case Instruction::fptosi:
                        // case Instruction::sitofp:
                        // case Instruction::getelementptr:
                        //     if (change_values.find(instr->get()) == change_values.end()) {
                        //         // motion_list.push_back(instr);
                        //         // if (instr->get_name() == "op10") {
                        //         //     LOG_DEBUG << instr->get_name();
                        //         // }
                        //         // inv_bb->add_instruction(*instr);
                        //         inv_bb->get_instructions().insert(pre_br, *instr);
                        //         (*instr)->set_parent(inv_bb);
                        //         instr = bb->get_instructions().erase(instr);
                        //     } else
                        //         instr++;
                        //     break;
                        case Instruction::ret:
                        case Instruction::br:
                        case Instruction::load:
                        case Instruction::phi:
                        // case Instruction::call:
                        case Instruction::mov:
                            instr++;
                            break;
                        case Instruction::store:
                            if (!is_store_movable(std::static_pointer_cast<StoreInst>(*instr), loop.get())) {
                                instr++;
                                break;
                            }
                        default:
                            if (change_values.find(instr->get()) == change_values.end()) {
                                inv_bb->get_instructions().insert(pre_br, *instr);
                                (*instr)->set_parent(inv_bb);
                                instr = bb->get_instructions().erase(instr);
                            } else
                                instr++;
                            break;
                    }
                }
                // for (auto instr : motion_list) {
                //     inv_bb->add_instruction(instr);
                //     bb->delete_instr(instr);
                //     instr->set_parent(inv_bb);
                // }
            }
        }
        // inv_bb->add_instruction(pre_br);
        // loop_pre->add_instruction(pre_br);
        // if (!inv_bb->get_instructions().empty()) {
        //     BranchInst::create(std::static_pointer_cast<BasicBlock>(base->shared_from_this()), inv_bb.get());
        //     inv_bb->add_pre_basic_block(loop_pre);
        //     inv_bb->add_succ_basic_block(base);
        //     base->get_pre_basic_blocks().remove(loop_pre);
        //     base->add_pre_basic_block(inv_bb.get());
        //     base->get_parent()->add_basic_block(inv_bb);
        //     auto pre_br = loop_pre->get_terminator();
        //     if (pre_br->get_num_operand() == 3) {
        //         // LOG_ERROR << "loop"
        //         for (int i = 1; i < 3; i++) {
        //             if (pre_br->get_operand(i).get() == base) {
        //                 pre_br->set_operand(i, std::static_pointer_cast<Value>(inv_bb));
        //             }
        //         }
        //     } else
        //         pre_br->set_operand(0, std::static_pointer_cast<Value>(inv_bb));
        //     loop_pre->get_succ_basic_blocks().remove(base);
        //     loop_pre->add_succ_basic_block(inv_bb.get());
        //     for (auto instr : base->get_instructions()) {
        //         if (instr->is_phi()) {
        //             for (int i = 0; i < instr->get_num_operand(); i++) {
        //                 if (instr->get_operand(i).get() == loop_pre) {
        //                     instr->set_operand(i, std::static_pointer_cast<Value>(inv_bb));
        //                 }
        //             }
        //         }
        //     }
        // }
        LOG_DEBUG << ++finished << " loop(s) finished";
    }
}

void LoopInvMotion::get_all_doms(BasicBlock *bb, std::unordered_set<BasicBlock *> &doms) {
    for (auto succ : _dom.get_dom_tree_succ_blocks(bb)) {
        doms.insert(succ);
        get_all_doms(succ, doms);
    }
}

std::vector<std::shared_ptr<bb_set_t>> LoopInvMotion::get_sub_loops(std::shared_ptr<bb_set_t> loop) {
    std::vector<std::shared_ptr<bb_set_t>> sub_loops;
    for (auto inner_loop : _loops) {
        if (inner_loop && _loops.get_parent_loop(inner_loop) == loop)
            sub_loops.push_back(inner_loop);
    }
    return sub_loops;
}