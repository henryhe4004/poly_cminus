#pragma once
#include "Module.h"
#include "Pass.hh"

class UselessOperationEli : public Pass {
  public:
    UselessOperationEli(Module *m) : Pass(m) {}
    void run() {
        for (auto func : m_->get_functions()) {
            for (auto bb : func->get_basic_blocks()) {
                for (auto instr_itr = bb->begin(); instr_itr != bb->end();) {
                    auto instr = *instr_itr;
                    std::shared_ptr<Value> new_val = nullptr;
                    if (instr->get_type()->is_integer_type()) {
                        if (instr->isBinary()) {
                            auto op1 = instr->get_operand(0);
                            auto op2 = instr->get_operand(1);
                            auto c1 = utils::get_const_int_val(op1.get());
                            auto c2 = utils::get_const_int_val(op2.get());
                            if (instr->is_add()) {
                                if (c1 == 0)
                                    new_val = op2->shared_from_this();
                                else if (c2 == 0)
                                    new_val = op1->shared_from_this();
                            } else if (instr->is_sub()) {
                                if (c2 == 0)
                                    new_val = op1->shared_from_this();
                            } else if (instr->is_mul()) {
                                if (c1 == 1)
                                    new_val = op2->shared_from_this();
                                else if (c2 == 1)
                                    new_val = op1->shared_from_this();
                                else if (c1 == 0 || c2 == 0) {
                                    new_val = ConstantInt::get(0, m_);
                                }
                            } else if (instr->is_div()) {
                                if (c2 == 1)
                                    new_val = op1->shared_from_this();
                                else if (c1 == 0)
                                    new_val = ConstantInt::get(0, m_);
                            } else if (instr->is_ashr() || instr->is_and_() || instr->is_lshr() || instr->is_shl()) {
                                if (c2 == 0)
                                    new_val = op1->shared_from_this();
                            }
                        }
                    }
                    if (new_val != nullptr) {
                        LOG_DEBUG << "UselessEli: try to replace " << instr->print() << " with " << new_val->print();
                        LOG_DEBUG << instr->print_usage();
                        LOG_DEBUG << new_val->print_usage();
                        instr->replace_all_use_with(new_val);
                        instr->remove_use_of_ops();
                        instr_itr = bb->get_instructions().erase(instr_itr);
                    } else
                        instr_itr++;
                }
            }
        }
    }
};
