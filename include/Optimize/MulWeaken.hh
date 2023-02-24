#pragma once
#include "Module.h"
#include "Pass.hh"
#include "Type.h"
#include "utils.hh"

class MulWeaken : public Pass {
  public:
    MulWeaken(Module *m) : Pass(m) {}
    void run() {
        for (auto func : m_->get_functions()) {
            for (auto bb : func->get_basic_blocks()) {
                for (auto instr_itr = bb->get_instructions().begin(); instr_itr != bb->get_instructions().end();
                     instr_itr++) {
                    auto instr = *instr_itr;
                    if (instr->is_mul() && (utils::is_const(instr->get_operand(0).get()) ||
                                            utils::is_const(instr->get_operand(1).get()))) {
                        int const_value;
                        Value *non_const_op;
                        if (utils::is_const(instr->get_operand(0).get())) {
                            const_value = static_cast<ConstantInt *>(instr->get_operand(0).get())->get_value();
                            non_const_op = instr->get_operand(1).get();
                        } else {
                            const_value = static_cast<ConstantInt *>(instr->get_operand(1).get())->get_value();
                            non_const_op = instr->get_operand(0).get();
                        }
                        if (const_value <= 0)
                            continue;
                        for (int sub : {0, -1, 1}) {
                            int const_value_ = const_value - sub;
                            if ((const_value_ & (const_value_ - 1)) == 0) {  // if const_value is power of 2
                                int pow = 0;
                                shared_ptr<Value> new_val;
                                for (; const_value_ >> (pow + 1); pow++)
                                    ;
                                if (pow == 0)
                                    new_val = non_const_op->shared_from_this();
                                else {
                                    std::shared_ptr<ConstantInt> shl_op_c;
                                    if (static_cast<IntegerType *>(instr->get_type())->get_num_bits() == 32)
                                        shl_op_c = ConstantInt::get(pow, m_);
                                    else
                                        shl_op_c = ConstantInt::get_i64(pow, m_);
                                    auto new_shl_inst = BinaryInst::create(instr->get_type(),
                                                                           Instruction::shl,
                                                                           non_const_op->shared_from_this(),
                                                                           shl_op_c,
                                                                           nullptr);
                                    bb->get_instructions().insert(instr_itr, new_shl_inst);
                                    new_shl_inst->set_parent(bb.get());
                                    if (sub == 1) {
                                        auto new_add_inst = BinaryInst::create(instr->get_type(),
                                                                               Instruction::add,
                                                                               new_shl_inst,
                                                                               non_const_op->shared_from_this(),
                                                                               nullptr);
                                        new_add_inst->set_parent(bb.get());
                                        new_val = new_add_inst;
                                        bb->get_instructions().insert(instr_itr, new_add_inst);
                                    } else if (sub == -1) {
                                        auto new_add_inst = BinaryInst::create(instr->get_type(),
                                                                               Instruction::sub,
                                                                               new_shl_inst,
                                                                               non_const_op->shared_from_this(),
                                                                               nullptr);
                                        new_add_inst->set_parent(bb.get());
                                        new_val = new_add_inst;
                                        bb->get_instructions().insert(instr_itr, new_add_inst);
                                    } else
                                        new_val = new_shl_inst;
                                }
                                instr->replace_all_use_with(new_val);
                                instr->get_use_list().clear();
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
};