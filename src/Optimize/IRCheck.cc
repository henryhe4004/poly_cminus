#include "Dominators.hh"
#include "Instruction.h"
#include "Pass.hh"
#include "utils.hh"

#include <algorithm>
#include <fstream>
#include <map>
#include <unordered_set>

void IRCheck::run() {
    check_parent();
    check_phi_position();
    check_ret_br_position();
    check_terminate();
    check_pred_succ();
    check_entry();
    check_operand_exit();
    check_use_list();
    check_pred_succ_br();
    // DONE 检查 phi 指令有没有被正确的维护
    check_correct_phi();
    check_ssa();
}
void IRCheck::check_ssa() {
    auto dom = std::make_unique<Dominators>(m_);
    dom->run();
    for (auto f : m_->get_functions()) {
        for (auto bb : f->get_basic_blocks()) {
            for (auto inst : bb->get_instructions()) {
                if (inst->is_phi())
                    continue;
                for (auto op : inst->get_operands()) {
                    auto op_inst = dynamic_cast<Instruction *>(op.get());
                    if (not op_inst)
                        continue;
                    auto op_bb = op_inst->get_parent();
                    if (bb.get() == op_bb) {
                        auto begin = bb->get_instructions().begin();
                        auto op_distance = std::distance(begin, op_inst->get_iterator());
                        auto cur_distance = std::distance(begin, inst->get_iterator());
                        if (not(op_distance < cur_distance)) {
                            exit(ERROR_IN_IR_CHECK, op_inst->get_name() + " does not dominates " + inst->get_name());
                        }
                    } else if (not dom->is_dominator(bb.get(), op_bb)) {
                        exit(ERROR_IN_IR_CHECK, "invalid SSA form");
                    }
                }
            }
        }
    }
}

void IRCheck::check_correct_phi() {
    for (auto func : m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            for (auto inst : bb->get_instructions()) {
                if (not inst->is_phi())
                    break;
                for (int i = 0; i < inst->get_num_operand(); i += 2) {
                    auto bb = std::dynamic_pointer_cast<BasicBlock>(inst->get_operand(i + 1)->shared_from_this());
                    if (std::find(func->begin(), func->end(), bb) == func->get_basic_blocks().end()) {
                        LOG_ERROR << "Wrong phi inst, inst " << inst->print() << " has a illegal pred "
                                  << bb->get_name();
                        exit(ERROR_IN_IR_CHECK);
                    }
                }
            }
        }
    }
}

void IRCheck::check_pred_succ_br() {
    for (auto func : m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            for (auto pred : bb->get_pre_basic_blocks()) {
                bool find = false;
                for (auto op : pred->get_terminator()->get_operands()) {
                    if (op->shared_from_this() == bb)
                        find = true;
                }
                if (not find) {
                    LOG_ERROR << "pred cannot reach current bb. pred: " << pred->get_name()
                              << " bb: " << bb->get_name();
                    exit(ERROR_IN_IR_CHECK);
                }
            }
            for (auto succ : bb->get_succ_basic_blocks()) {
                bool find = false;
                for (auto op : bb->get_terminator()->get_operands()) {
                    if (op->shared_from_this() == succ->shared_from_this())
                        find = true;
                }
                if (not find) {
                    LOG_ERROR << "succ is not in current bb's branch list";
                    exit(ERROR_IN_IR_CHECK);
                }
            }
            for (auto op : bb->get_terminator()->get_operands()) {
                if (auto succ = std::dynamic_pointer_cast<BasicBlock>(op->shared_from_this())) {
                    if (std::find(bb->get_succ_basic_blocks().begin(), bb->get_succ_basic_blocks().end(), succ.get()) ==
                        bb->get_succ_basic_blocks().end()) {
                        LOG_ERROR << "bb's branch target does not belong to its succ list. bb: " << bb->get_name()
                                  << " -> " << succ->get_name();
                        exit(ERROR_IN_IR_CHECK);
                    }
                }
            }
        }
    }
}

void IRCheck::check_parent() {
    for (auto func : m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            if (bb->get_parent() == nullptr) {
                LOG(ERROR) << "bb with no parent";
                throw std::exception();
            }
            if (bb->get_parent() != &*func) {
                LOG(ERROR) << "bb with wrong parent";
                throw std::exception();
            }
            for (auto instr : bb->get_instructions()) {
                if (instr->get_parent() == nullptr) {
                    LOG(ERROR) << instr->print();
                    LOG(ERROR) << "instr with no parent";
                    throw std::exception();
                }
                if (instr->get_parent() != &*bb) {
                    LOG(ERROR) << instr->print();
                    LOG(ERROR) << "instr with wrong parent";
                    throw std::exception();
                }
                if (instr->get_function() == nullptr) {
                    LOG(ERROR) << instr->print();
                    LOG(ERROR) << "instr with no function";
                    throw std::exception();
                }
                if (instr->get_function() != &*func) {
                    LOG(ERROR) << instr->print();
                    LOG(ERROR) << "instr with wrong function";
                    throw std::exception();
                }
            }
        }
    }
}

void IRCheck::check_phi_position() {
    for (auto func : m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            bool pos_begin = true;
            for (auto instr : bb->get_instructions()) {
                if (instr->is_phi()) {
                    if (pos_begin) {
                        continue;
                    } else {
                        LOG(ERROR) << "phi postion error";
                        throw std::exception();
                    }
                } else {
                    pos_begin = false;
                }
            }
        }
    }
}

void IRCheck::check_ret_br_position() {
    for (auto func : m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            for (auto instr : bb->get_instructions()) {
                if (instr->is_br() || instr->is_ret()) {
                    if (instr != bb->get_terminator()) {
                        LOG(ERROR) << "bb <label>%" << bb->get_name() << " error in br ret position";
                        throw std::exception();
                    }
                }
            }
        }
    }
}

void IRCheck::check_terminate() {
    for (auto func : m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            if (bb->get_terminator() == nullptr) {
                LOG(ERROR) << "bb <label>%" << bb->get_name() << " doesn't have terminator";
                throw std::exception();
            }
        }
    }
}

void IRCheck::check_pred_succ() {
    for (auto func : m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            // pred
            std::set<BasicBlock*> check_bb_pre(bb->get_pre_basic_blocks().begin(), bb->get_pre_basic_blocks().end());
            if (check_bb_pre.size() != bb->get_pre_basic_blocks().size()) {
                LOG_ERROR << "duplicate blocks " << bb->get_name();
                throw std::exception();
            }
            for (auto pre_bb : bb->get_pre_basic_blocks()) {
                bool find = false;
                for (auto pre_bb_succ : pre_bb->get_succ_basic_blocks()) {
                    if (pre_bb_succ == &*bb) {
                        find = true;
                    }
                }
                if (!find) {
                    LOG(ERROR) << "error in bb pre & succ" << bb->get_name();
                    throw std::exception();
                }
            }
            // succ
            std::set<BasicBlock*> check_bb_succ(bb->get_succ_basic_blocks().begin(), bb->get_succ_basic_blocks().end());
            if (check_bb_succ.size() != bb->get_succ_basic_blocks().size()) {
                LOG_ERROR << "duplicate blocks " << bb->get_name();
                throw std::exception();
            }
            for (auto succ_bb : bb->get_succ_basic_blocks()) {
                bool find = false;
                for (auto succ_bb_pre : succ_bb->get_pre_basic_blocks()) {
                    if (succ_bb_pre == &*bb) {
                        find = true;
                    }
                }
                if (!find) {
                    LOG(ERROR) << "error in bb pre & succ: " << bb->get_name();
                    throw std::exception();
                }
            }
        }
    }
}

void IRCheck::check_entry() {
    for (auto &func : m_->get_functions()) {
        if (!func->get_basic_blocks().empty()) {
            if (func->get_entry_block() != func->get_basic_blocks().front()) {
                LOG(ERROR) << "entry block is not the first block";
                throw std::exception();
            }
        }
    }
}

void IRCheck::check_use_list() {
    std::unordered_set<Value *> all_users;
    for (auto func : m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            for (auto inst : bb->get_instructions()) {
                all_users.insert(inst.get());
            }
        }
    }
    for (auto global : m_->get_global_variables()) {
        // check global variable
        auto uses = global->get_use_list();
        auto uniq = std::unordered_set<Use, UseHash>(uses.begin(), uses.end());
        if (uniq.size() != uses.size()) {
            LOG(ERROR) << "global @" << global->get_name() << " has duplicate use";
            throw std::exception();
        }
        for (auto use : uses) {
            if (all_users.find(use.val_) == all_users.end()) {
                LOG_ERROR << "global @" << global->get_name() << " has an undefined user";
                throw std::exception();
            }
            auto val = dynamic_cast<User *>(use.val_);
            if (!val) {
                LOG(ERROR) << "global @" << global->get_name() << " has non-User type use";
                throw std::exception();
            }
            if (val->get_operand(use.arg_no_).get() != global.get()) {
                LOG(ERROR) << "value %" << val->get_name() << " don't use global @" << global->get_name();
                throw std::exception();
            }
        }
    }
    for (auto func : m_->get_functions()) {
        // check function
        auto uses = func->get_use_list();
        auto uniq = std::unordered_set<Use, UseHash>(uses.begin(), uses.end());
        if (uniq.size() != uses.size()) {
            LOG(ERROR) << "func @" << func->get_name() << " has duplicate use";
            throw std::exception();
        }
        for (auto use : uses) {
            if (all_users.find(use.val_) == all_users.end()) {
                LOG_ERROR << "func @" << func->get_name() << " has an undefined user";
                throw std::exception();
            }
            auto val = dynamic_cast<User *>(use.val_);
            if (!val) {
                LOG(ERROR) << "func @" << func->get_name() << " has non-User type use";
                throw std::exception();
            }
            if (val->get_operand(use.arg_no_).get() != func.get()) {
                LOG(ERROR) << "value %" << val->get_name() << " don't use func @" << func->get_name();
                throw std::exception();
            }
        }
        for (auto arg : func->get_args()) {
            // check argument
            auto uses = arg->get_use_list();
            auto uniq = std::unordered_set<Use, UseHash>(uses.begin(), uses.end());
            if (uniq.size() != uses.size()) {
                LOG(ERROR) << "arg %" << arg->get_name() << " has duplicate use";
                throw std::exception();
            }
            for (auto use : uses) {
                if (all_users.find(use.val_) == all_users.end()) {
                    LOG_ERROR << "arg %" << arg->get_name() << " has an undefined user";
                    throw std::exception();
                }
                auto val = dynamic_cast<User *>(use.val_);
                if (!val) {
                    LOG(ERROR) << "arg %" << arg->get_name() << " has non-User type use";
                    throw std::exception();
                }
                if (val->get_operand(use.arg_no_).get() != arg.get()) {
                    LOG(ERROR) << "value %" << val->get_name() << " don't use arg %" << arg->get_name();
                    throw std::exception();
                }
            }
        }
        for (auto bb : func->get_basic_blocks()) {
            // check basicblock
            auto uses = bb->get_use_list();
            auto uniq = std::unordered_set<Use, UseHash>(uses.begin(), uses.end());
            if (uniq.size() != uses.size()) {
                LOG(ERROR) << "bb <label>%" << bb->get_name() << " has duplicate use";
                throw std::exception();
            }
            for (auto use : uses) {
                if (all_users.find(use.val_) == all_users.end()) {
                    LOG_ERROR << "bb <label>%" << bb->get_name() << " has an undefined user";
                    throw std::exception();
                }
                auto val = dynamic_cast<User *>(use.val_);
                if (!val) {
                    LOG(ERROR) << "bb <label>%" << bb->get_name() << " has non-User type use";
                    throw std::exception();
                }
                if (val->get_operand(use.arg_no_).get() != bb.get()) {
                    LOG(ERROR) << "value %" << val->get_name() << " don't use bb <label>%" << bb->get_name() << " "
                               << val->print();
                    throw std::exception();
                }
            }
            for (auto inst : bb->get_instructions()) {
                // check instructions
                auto uses = inst->get_use_list();
                auto uniq = std::unordered_set<Use, UseHash>(uses.begin(), uses.end());
                if (uniq.size() != uses.size()) {
                    LOG(ERROR) << "inst %" << inst->get_name() << " has duplicate use";
                    throw std::exception();
                }
                for (auto use : uses) {
                    if (all_users.find(use.val_) == all_users.end()) {
                        LOG_ERROR << "inst %" << inst->get_name() << " has an undefined user";
                        throw std::exception();
                    }
                    auto val = dynamic_cast<User *>(use.val_);
                    if (!val) {
                        LOG(ERROR) << "inst %" << inst->get_name() << " has non-User type use";
                        throw std::exception();
                    }
                    if (val->get_operand(use.arg_no_).get() != inst.get()) {
                        LOG(ERROR) << "value %" << val->get_name() << " don't use inst %" << inst->get_name();
                        throw std::exception();
                    }
                }
                // 这里太慢了
                // int index = 0;
                // for (auto op : inst->get_operands()) {
                //     auto uses = op->get_use_list();
                //     auto iter = std::find(uses.begin(), uses.end(), Use(inst.get(), index));
                //     if (iter == uses.end()) {
                //         LOG(ERROR) << "inst " << inst->print() << " is not in use list of operand " << op->print();
                //         LOG_ERROR << op->print_usage();
                //         throw std::exception();
                //     }
                //     index++;
                // }
            }
        }
    }
}

void IRCheck::check_operand_exit() {
    std::unordered_set<Value *> global_defined;
    for (auto var : m_->get_global_variables())
        global_defined.insert(var.get());
    for (auto func : m_->get_functions())
        global_defined.insert(func.get());
    for (auto func : m_->get_functions()) {
        std::unordered_set<Value *> defined;
        for (auto arg : func->get_args())
            defined.insert(arg.get());
        for (auto bb : func->get_basic_blocks()) {
            defined.insert(bb.get());
            for (auto inst : bb->get_instructions()) {
                defined.insert(inst.get());
            }
        }
        for (auto bb : func->get_basic_blocks()) {
            for (auto inst : bb->get_instructions()) {
                for (auto op : inst->get_operands()) {
                    if (dynamic_cast<Constant *>(op.get()) || defined.find(op.get()) != defined.end() ||
                        global_defined.find(op.get()) != global_defined.end())
                        continue;
                    else {
                        LOG(ERROR) << inst->print();
                        LOG(ERROR) << "inst %" << inst->get_name() << " has undefined operand %" << op->get_name();
                        throw std::exception();
                    }
                }
            }
        }
    }
}
