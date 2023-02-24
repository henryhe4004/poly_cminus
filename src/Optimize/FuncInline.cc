#include "FuncInline.hh"

#include <queue>
#include <unordered_set>

void FuncInline::run() {
    // debug
    // std::cout << m_->print();
    for (auto func : m_->get_functions()) {
        if (is_inlineable(func.get())) {
            for (auto use : func->get_use_list()) {
                auto caller_inst = std::dynamic_pointer_cast<CallInst>(use.val_->shared_from_this());
                auto caller_bb = caller_inst->get_parent();
                auto caller_func = caller_bb->get_parent();
                if (!caller_inst) {
                    exit(219);
                    LOG_ERROR << "function " << func->get_name() << " has a non-CallInst user " << use.val_->get_name();
                }
                // std::unordered_map<Value *, Value *> val_old2new;

                // debug
                // if (caller_inst->get_name() == "op59") {
                //     std::cout << m_->print();
                // }

                // 将形参-实参对加入旧-新值映射
                auto form_para_itr = func->get_args().begin();
                for (int i = 1; i < func->get_num_of_args() + 1; i++) {
                    val_old2new.insert({form_para_itr->get(), caller_inst->get_operand(i).get()});
                    form_para_itr++;
                }

                // 找到调用指令的迭代器
                // std::list<shared_ptr<Instruction>>::iterator caller_itr;
                // for (caller_itr = caller_bb->get_instructions().begin();
                //      caller_itr != caller_bb->get_instructions().end();
                //      caller_itr++) {
                //     if (caller_itr->get() == caller_inst)
                //         break;
                // }
                auto caller_itr = caller_bb->find_instr(caller_inst);

                // 创建内联后返回的块，并将调用指令后的指令移动至该新块
                auto returned_bb = BasicBlock::create(m_, "returned" + std::to_string(counter++), caller_func);
                auto after_caller_itr = caller_itr;
                after_caller_itr++;
                for (; after_caller_itr != caller_bb->get_instructions().end();) {
                    returned_bb->add_instruction((*after_caller_itr));
                    (*after_caller_itr)->set_parent(returned_bb.get());
                    after_caller_itr = caller_bb->get_instructions().erase(after_caller_itr);
                }
                auto returned_ter = returned_bb->get_terminator();
                if (!returned_ter->is_ret()) {
                    // returned_bb->get_instructions().pop_back();
                    auto returned_br = std::static_pointer_cast<BranchInst>(returned_ter);
                    if (returned_br->is_cond_br()) {
                        // BranchInst::create_cond_br(
                        //     returned_br->get_operand(0).get()->shared_from_this(),
                        //     std::static_pointer_cast<BasicBlock>(returned_br->get_operand(1).get()->shared_from_this()),
                        //     std::static_pointer_cast<BasicBlock>(returned_br->get_operand(2).get()->shared_from_this()),
                        //     returned_bb.get());

                        auto true_bb = static_cast<BasicBlock *>(returned_br->get_operand(1).get());
                        auto false_bb = static_cast<BasicBlock *>(returned_br->get_operand(2).get());
                        true_bb->add_pre_basic_block(returned_bb.get());
                        false_bb->add_pre_basic_block(returned_bb.get());
                        returned_bb->add_succ_basic_block(true_bb);
                        returned_bb->add_succ_basic_block(false_bb);
                    } else {
                        // BranchInst::create_br(
                        //     std::static_pointer_cast<BasicBlock>(returned_br->get_operand(0).get()->shared_from_this()),
                        //     returned_bb.get());
                        auto true_bb = static_cast<BasicBlock *>(returned_br->get_operand(0).get());
                        true_bb->add_pre_basic_block(returned_bb.get());
                        returned_bb->add_succ_basic_block(true_bb);
                    }
                }
                for (auto caller_bb_succ : caller_bb->get_succ_basic_blocks())
                    caller_bb_succ->remove_pre_basic_block(caller_bb);
                caller_bb->clear_succ_basic_blocks();

                // 删除调用指令
                caller_bb->get_instructions().erase(caller_itr);

                // 将原本是caller_bb的phi指令操作数替换为returned_bb
                for (auto succ_bb : returned_bb->get_succ_basic_blocks()) {
                    for (auto phi : succ_bb->get_instructions()) {
                        if (!phi->is_phi())
                            break;
                        for (int i = 1; i < phi->get_num_operand(); i += 2) {
                            if (phi->get_operand(i).get() == caller_bb) {
                                phi->set_operand(i, returned_bb->weak_from_this());
                                caller_bb->remove_use(phi.get());
                            }
                        }
                    }
                }
                // 每一条返回语句的值和所在基本块，用于在返回的块开头创建phi指令
                vector<std::pair<std::weak_ptr<Value>, shared_ptr<BasicBlock>>> return_phi_pair_ops;

                // 创建被调函数的基本块的迭代器，记录旧的entry块
                std::list<shared_ptr<BasicBlock>>::iterator func_bb_itr;
                func_bb_itr = func->get_basic_blocks().begin();
                auto old_entry = func->get_entry_block();

                val_old2new.insert({old_entry.get(), caller_bb});

                // 旧的跳转指令的列表
                // 因为在处理某条跳转指令时，其目标的新块可能尚未创建，
                // 因此先将所有跳转指令加入一个列表中最后集中处理
                vector<shared_ptr<BranchInst>> old_br_list;

                // 旧的phi指令的列表
                // 作用同上，最后添加操作数
                vector<shared_ptr<PhiInst>> old_phi_list;

                // 遍历被调函数的块和指令，生成新的块和指令
                // 后继块的广度优先图遍历
                std::queue<shared_ptr<BasicBlock>> block_queue;
                std::unordered_set<shared_ptr<BasicBlock>> visited;
                block_queue.push(old_entry);
                visited.insert(old_entry);
                while (!block_queue.empty()) {
                    auto old_bb = block_queue.front();
                    block_queue.pop();
                    // visited.insert(old_bb);
                    for (auto succ : old_bb->get_succ_basic_blocks()) {
                        auto succ_shared = std::static_pointer_cast<BasicBlock>(succ->shared_from_this());
                        if (visited.find(succ_shared) == visited.end()) {
                            block_queue.push(succ_shared);
                            visited.insert(succ_shared);
                        }
                    }
                    // for (; func_bb_itr != func->get_basic_blocks().end(); func_bb_itr++) {
                    // 创建对应的新块
                    shared_ptr<BasicBlock> new_bb;
                    if (old_bb == old_entry)
                        new_bb = std::static_pointer_cast<BasicBlock>(caller_bb->shared_from_this());
                    else {
                        new_bb = BasicBlock::create(
                            m_, func->get_name() + old_bb->get_name() + std::to_string(counter++), caller_func);
                        val_old2new.insert({old_bb.get(), new_bb.get()});
                    }

                    // 遍历旧指令
                    for (auto old_inst : old_bb->get_instructions()) {
                        shared_ptr<Instruction> new_inst;

                        // 分情况创建新指令
                        if (old_inst->is_ret()) {
                            // 非void的return语句，将值和块加入列表
                            if (old_inst->get_num_operand() == 1) {
                                return_phi_pair_ops.push_back({get_new_op(old_inst, 0)->weak_from_this(), new_bb});
                            }
                            BranchInst::create_br(returned_bb, new_bb.get());
                        } else if (old_inst->isBinary()) {
                            new_inst = BinaryInst::create(old_inst->get_type(),
                                                          old_inst->get_instr_type(),
                                                          get_new_op(old_inst, 0)->shared_from_this(),
                                                          get_new_op(old_inst, 1)->shared_from_this(),
                                                          new_bb.get());
                        } else if (old_inst->is_cmp()) {
                            new_inst = CmpInst::create_cmp(static_cast<CmpInst *>(old_inst.get())->get_cmp_op(),
                                                           get_new_op(old_inst, 0)->shared_from_this(),
                                                           get_new_op(old_inst, 1)->shared_from_this(),
                                                           new_bb.get());
                        } else if (old_inst->is_fcmp()) {
                            new_inst = FCmpInst::create_fcmp(static_cast<FCmpInst *>(old_inst.get())->get_cmp_op(),
                                                             get_new_op(old_inst, 0)->shared_from_this(),
                                                             get_new_op(old_inst, 1)->shared_from_this(),
                                                             new_bb.get());
                        } else if (old_inst->is_call()) {
                            vector<shared_ptr<Value>> new_arg_list;
                            for (int i = 1; i < old_inst->get_num_operand(); i++) {
                                new_arg_list.push_back(get_new_op(old_inst, i)->shared_from_this());
                            }
                            auto func_shared =
                                std::static_pointer_cast<Function>(old_inst->get_operand(0).get()->shared_from_this());
                            new_inst = CallInst::create(func_shared, new_arg_list, new_bb.get());
                        } else if (old_inst->is_br()) {
                            old_br_list.push_back(std::static_pointer_cast<BranchInst>(old_inst));
                        } else if (old_inst->is_gep()) {
                            vector<shared_ptr<Value>> idxs;
                            for (int i = 1; i < old_inst->get_num_operand(); i++) {
                                idxs.push_back(get_new_op(old_inst, i)->shared_from_this());
                            }
                            new_inst = GetElementPtrInst::create_gep(
                                get_new_op(old_inst, 0)->shared_from_this(), idxs, new_bb.get());
                        } else if (old_inst->is_store()) {
                            new_inst = StoreInst::create_store(get_new_op(old_inst, 0)->shared_from_this(),
                                                               get_new_op(old_inst, 1)->shared_from_this(),
                                                               new_bb.get());
                        } else if (old_inst->is_load()) {
                            new_inst = LoadInst::create_load(
                                old_inst->get_type(), get_new_op(old_inst, 0)->shared_from_this(), new_bb.get());
                        } else if (old_inst->is_alloca()) {
                            if (!caller_func->get_entry_block()->empty()) {
                                auto insert_point = caller_func->get_entry_block()->begin();
                                auto tt = *insert_point;
                                new_inst = AllocaInst::create(
                                    static_cast<AllocaInst *>(old_inst.get())->get_alloca_type(), &insert_point);
                            } else
                                new_inst = AllocaInst::create_alloca(
                                    static_cast<AllocaInst *>(old_inst.get())->get_alloca_type(), new_bb.get());
                        } else if (old_inst->is_zext()) {
                            new_inst = ZextInst::create_zext(get_new_op(old_inst, 0)->shared_from_this(),
                                                             static_cast<ZextInst *>(old_inst.get())->get_dest_type(),
                                                             new_bb.get());
                        } else if (old_inst->is_si2fp()) {
                            new_inst = SiToFpInst::create_sitofp(
                                get_new_op(old_inst, 0)->shared_from_this(), old_inst->get_type(), new_bb.get());
                        } else if (old_inst->is_fp2si()) {
                            new_inst = FpToSiInst::create_fptosi(
                                get_new_op(old_inst, 0)->shared_from_this(), old_inst->get_type(), new_bb.get());
                        } else if (old_inst->is_phi()) {
                            // auto new_phi = PhiInst::create_phi(old_inst->get_type(), new_bb.get());
                            new_inst = PhiInst::create_phi(old_inst->get_type(), new_bb.get());
                            old_phi_list.push_back(std::static_pointer_cast<PhiInst>(old_inst));
                        } else if (old_inst->is_inttoptr()) {
                            new_inst = IntToPtrInst::create_inttoptr(
                                get_new_op(old_inst, 0)->shared_from_this(), old_inst->get_type(), new_bb.get());
                        } else if (old_inst->is_ptrtoint()) {
                            new_inst = PtrToIntInst::create_ptrtoint(get_new_op(old_inst, 0)->shared_from_this(),
                                                                     new_bb.get());
                        } else {
                            exit(220);
                            LOG_ERROR << "has not finished";
                        }
                        val_old2new.insert({old_inst.get(), new_inst.get()});
                    }
                }
                // 添加Branch指令
                for (auto old_br : old_br_list) {
                    auto new_bb = static_cast<BasicBlock *>(get_new_val(old_br->get_parent()));
                    shared_ptr<BranchInst> new_br;
                    if (old_br->is_cond_br()) {
                        new_br = BranchInst::create_cond_br(
                            get_new_op(old_br, 0)->shared_from_this(),
                            std::static_pointer_cast<BasicBlock>(get_new_op(old_br, 1)->shared_from_this()),
                            std::static_pointer_cast<BasicBlock>(get_new_op(old_br, 2)->shared_from_this()),
                            new_bb);
                    } else {
                        new_br = BranchInst::create_br(
                            std::static_pointer_cast<BasicBlock>(get_new_op(old_br, 0)->shared_from_this()), new_bb);
                    }
                }

                // 给phi指令补充操作数
                for (auto old_phi : old_phi_list) {
                    auto new_phi = std::static_pointer_cast<PhiInst>(get_new_val(old_phi.get())->shared_from_this());
                    for (int i = 0; i < old_phi->get_num_operand(); i += 2) {
                        new_phi->add_phi_pair_operand(
                            get_new_op(old_phi, i)->weak_from_this(),
                            std::static_pointer_cast<BasicBlock>(get_new_op(old_phi, i + 1)->shared_from_this()));
                    }
                }

                // 在返回的块添加phi指令，用以替代call指令
                if (!return_phi_pair_ops.empty()) {
                    shared_ptr<Value> new_caller_val;
                    if (return_phi_pair_ops.size() == 1) {
                        new_caller_val = return_phi_pair_ops[0].first.lock();
                    } else {
                        auto phi_after_func = PhiInst::create_phi(func->get_return_type());
                        returned_bb->add_instr_begin(phi_after_func);
                        phi_after_func->set_parent(returned_bb.get());
                        for (auto pair_op : return_phi_pair_ops)
                            phi_after_func->add_phi_pair_operand(pair_op.first, pair_op.second);
                        new_caller_val = phi_after_func;
                    }
                    caller_inst->replace_all_use_with(new_caller_val);
                }
                // caller_inst->remove_use_of_ops();
                // 移除调用指令的操作数的相关use信息，除了第一个操作数
                // （即函数本身，迭代中不应直接修改，迭代完后统一删除）
                for (int i = 1; i < caller_inst->get_num_operand(); i++)
                    caller_inst->get_operand(i)->remove_use(caller_inst.get());

                // 删除调用指令，并在其所在块添加br
                // caller_bb->get_instructions().erase(caller_itr);
                // auto inlined_entry =
                //     std::static_pointer_cast<BasicBlock>(get_new_val(old_entry.get())->shared_from_this());
                // BranchInst::create_br(inlined_entry, caller_bb);

                // 内联完成，清除新旧值映射信息
                val_old2new.clear();
            }
            func->get_use_list().clear();

            LOG_DEBUG << "func " << func->get_name() << " has been inlined";
        }
    }
    // debug
    // std::cout << m_->print();
}

bool FuncInline::is_inlineable(Function *func) {
    // 仅有声明
    if (func->is_declaration())
        return false;

    // 主函数
    if (func->get_name() == "main")
        return false;

    // 递归调用
    for (auto use : func->get_use_list()) {
        if (static_cast<Instruction *>(use.val_)->get_parent()->get_parent() == func)
            return false;
    }

    // 不考虑语句数量，做十分激进的函数内联
    return true;

    // 语句数量
    int inst_num = 0;
    for (auto bb : func->get_basic_blocks())
        inst_num += bb->get_instructions().size();
    if (inst_num > MAX_INSTRUCTION_NUM)
        return false;
    return true;
}
