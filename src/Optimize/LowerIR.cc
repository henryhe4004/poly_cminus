#include "LowerIR.hh"

#include "lir.h"
#include "utils.hh"

/** TODO:
 * 1. 考虑这样的代码：
 * while (i) i = i - 1;
 * 假设i分到零号寄存器可以fuse成
 * subs r0, r0, #1
 * bne ...
 * 省去了icmp的ne done.
 * 2. 有内存操作时，归纳变量可以在load或store时进行增减
 * 3. ashr和shl的操作可以合成一个bic
 * 4. 除常数要不要在这里做？
 * 5. load/store时的常数偏移量 done.
 */

void LowerIR::run() {
    fuse_mul_add();
    fuse_const_shift();
    fuse_memory_offset();
    // run CFGsimplification before lowering to create more fused branches
    fuse_br();
    // looks like there's no problem with this
    delete_ptr_int_conversions();
    m_->level = Module::LIR;
}

void LowerIR::fuse_br() {
    for (auto f : m_->get_functions())
        for (auto bb : f->get_basic_blocks()) {
            auto term = bb->get_terminator();
            if (term->is_br() and term->get_num_operand() > 1) {
                auto cond = dynamic_cast<CmpInst *>(term->get_operand(0).get());
                if (not cond)
                    continue;
                if (utils::is_const_int(cond->get_operand(1).get(), 0)) {
                    auto other = cond->get_operand(0).get();
                    // TODO: better ensure that no other compare instructions are in the same block
                    if (!(isa<BinaryInst>(other) and dynamic_cast<BinaryInst *>(other)->get_parent() == bb.get()) or
                        dynamic_cast<BinaryInst *>(other)->is_div() or dynamic_cast<BinaryInst *>(other)->is_srem())
                        continue;

                    // term->remove_operands(0, 0);
                    cond->remove_use(term.get());
                    term->set_operand(0, other->shared_from_this());  // 以免dce消除了该指令，TODO:
                                                                      // 寄存器分配时可以不在branch这里调用add_use
                    dynamic_cast<BinaryInst *>(other)->set_flag();
                    std::dynamic_pointer_cast<BranchInst>(term)->set_cmp_op(cond->get_cmp_op());
                }
            }
        }
}

void LowerIR::fuse_memory_offset() {
    for (auto f : m_->get_functions())
        for (auto bb : f->get_basic_blocks())
            for (auto it = bb->begin(); it != bb->end(); ++it) {  // 起手式...
                auto i = it->get();
                Value *base_ptr;
                Value *base_lhs, *base_rhs;
                if (i->is_load() or i->is_store()) {
                    size_t ptr_index = i->is_load() ? 0 : 1;
                    bool is_float =
                        i->is_load() ? i->get_type()->is_float_type() : i->get_operand(0)->get_type()->is_float_type();
                    base_ptr = i->get_operand(ptr_index).get();
                    if (match(base_ptr, m_inttoptr(m_Add(m_value(base_lhs), m_value(base_rhs))))) {
                        if (isa<Constant>(base_lhs))
                            std::swap(base_lhs, base_rhs);
                        if (AllocaInst * _; match(base_rhs, m_ptrtoint(m_alloca(_))))
                            std::swap(base_lhs, base_rhs);
                        if (!isa<Constant>(base_rhs) and is_float)  // vldr/vstr不支持寄存器偏移
                            continue;
                        auto add_inst =
                            dynamic_cast<BinaryInst *>(dynamic_cast<IntToPtrInst *>(base_ptr)->get_operand(0).get());
                        i->remove_operands(ptr_index, ptr_index);
                        i->add_operand(base_lhs->shared_from_this());
                        i->add_operand(base_rhs->shared_from_this());
                        // 浮点的rhs是常数，不会有 operand2
                        if (auto load = dynamic_cast<LoadInst *>(i))
                            load->set_op2_shift(add_inst->get_op2_shift_type(), add_inst->get_op2_shift_bits());
                        else if (auto store = dynamic_cast<StoreInst *>(i))
                            store->set_op2_shift(add_inst->get_op2_shift_type(), add_inst->get_op2_shift_bits());
                    }
                }
            }
}

// warning: this produces non type safe IR, don't open `-lower-ir` when using llvm toolchains
void LowerIR::delete_ptr_int_conversions() {
    for (auto f : m_->get_functions())
        for (auto bb : f->get_basic_blocks())
            for (auto it = bb->begin(); it != bb->end(); ++it) {
                auto i = it->get();
                Value *v;
                if (match(i, m_inttoptr(m_value(v))) or
                    (match(i, m_ptrtoint(m_value(v))) and !dynamic_cast<GlobalVariable *>(v)))
                    i->replace_all_use_with(v->shared_from_this());
            }
}

void LowerIR::fuse_mul_add() {
    for (auto func : m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            for (auto inst : *bb) {
                Value *add_op;
                Value *mul_op1;
                Value *mul_op2;
                if ((match(inst.get(), m_Add(m_Mul(m_value(mul_op1), m_value(mul_op2)), m_value(add_op)))) or
                    (match(inst.get(), m_Add(m_value(add_op), m_Mul(m_value(mul_op1), m_value(mul_op2)))))) {
                    auto inst_it = bb->find_instr(inst);
                    auto new_mula_inst = MuladdInst::create_muladd(
                        mul_op1->shared_from_this(), mul_op2->shared_from_this(), add_op->shared_from_this(), &inst_it);
                    inst->replace_all_use_with(new_mula_inst);
                } else if (match(inst.get(), m_Sub(m_value(add_op), m_Mul(m_value(mul_op1), m_value(mul_op2))))) {
                    auto inst_it = bb->find_instr(inst);
                    auto new_mula_inst = MulsubInst::create_mulsub(
                        mul_op1->shared_from_this(), mul_op2->shared_from_this(), add_op->shared_from_this(), &inst_it);
                    inst->replace_all_use_with(new_mula_inst);
                }
            }
        }
    }
}

void LowerIR::fuse_const_shift() {
    for (auto func : m_->get_functions()) {
        for (auto bb : func->get_basic_blocks()) {
            for (auto inst_it = bb->begin(); inst_it != bb->end(); inst_it++) {
                auto inst = *inst_it;
                std::shared_ptr<BinaryInst> new_inst;
                if (inst->is_add()) {
                    Value *op1, *op2;
                    ConstantInt *c_sh;
                    if (match(inst.get(), m_Add(m_value(op1), m_Shl(m_value(op2), m_constantInt(c_sh))))) {
                        new_inst = BinaryInst::create_add(op1->shared_from_this(), op2->shared_from_this(), &inst_it);
                        new_inst->set_op2_shift(shift_type_t::LSL, c_sh->get_value());
                    } else if (match(inst.get(), m_Add(m_value(op1), m_Ashr(m_value(op2), m_constantInt(c_sh))))) {
                        new_inst = BinaryInst::create_add(op1->shared_from_this(), op2->shared_from_this(), &inst_it);
                        new_inst->set_op2_shift(shift_type_t::ASR, c_sh->get_value());
                    } else if (match(inst.get(), m_Add(m_value(op1), m_Lshr(m_value(op2), m_constantInt(c_sh))))) {
                        new_inst = BinaryInst::create_add(op1->shared_from_this(), op2->shared_from_this(), &inst_it);
                        new_inst->set_op2_shift(shift_type_t::LSR, c_sh->get_value());
                    }
                } else if (inst->is_sub()) {
                    auto sub_op1 = inst->get_operand(0).get();
                    auto sub_op2 = inst->get_operand(1).get();
                    Value *sh_op1;
                    ConstantInt *c_sh;
                    if (match(sub_op2, m_Shl(m_value(sh_op1), m_constantInt(c_sh)))) {
                        new_inst =
                            BinaryInst::create_sub(sub_op1->shared_from_this(), sh_op1->shared_from_this(), &inst_it);
                        new_inst->set_op2_shift(shift_type_t::LSL, c_sh->get_value());
                    } else if (match(sub_op2, m_Ashr(m_value(sh_op1), m_constantInt(c_sh)))) {
                        new_inst =
                            BinaryInst::create_sub(sub_op1->shared_from_this(), sh_op1->shared_from_this(), &inst_it);
                        new_inst->set_op2_shift(shift_type_t::ASR, c_sh->get_value());
                    } else if (match(sub_op2, m_Lshr(m_value(sh_op1), m_constantInt(c_sh)))) {
                        new_inst =
                            BinaryInst::create_sub(sub_op1->shared_from_this(), sh_op1->shared_from_this(), &inst_it);
                        new_inst->set_op2_shift(shift_type_t::LSR, c_sh->get_value());
                    } else if (match(sub_op1, m_Shl(m_value(sh_op1), m_constantInt(c_sh)))) {
                        new_inst =
                            BinaryInst::create_rsub(sub_op2->shared_from_this(), sh_op1->shared_from_this(), &inst_it);
                        new_inst->set_op2_shift(shift_type_t::LSL, c_sh->get_value());
                    } else if (match(sub_op1, m_Ashr(m_value(sh_op1), m_constantInt(c_sh)))) {
                        new_inst =
                            BinaryInst::create_rsub(sub_op2->shared_from_this(), sh_op1->shared_from_this(), &inst_it);
                        new_inst->set_op2_shift(shift_type_t::ASR, c_sh->get_value());
                    } else if (match(sub_op1, m_Lshr(m_value(sh_op1), m_constantInt(c_sh)))) {
                        new_inst =
                            BinaryInst::create_rsub(sub_op2->shared_from_this(), sh_op1->shared_from_this(), &inst_it);
                        new_inst->set_op2_shift(shift_type_t::LSR, c_sh->get_value());
                    }
                }
                if (new_inst != nullptr) {
                    inst->replace_all_use_with(new_inst);
                }
            }
        }
    }
}