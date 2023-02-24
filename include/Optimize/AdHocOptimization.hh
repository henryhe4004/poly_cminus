#pragma once
#include "BasicBlock.h"
#include "Pass.hh"
#include "utils.hh"
#include "LoopInfo.hh"

// ad hoc optimization, 特设优化
//（有ad hoc polymorphism就可以有ad hoc optimization!）
// 1. memset/memcpy
// 2. bit manipulation b/c the stupid sysy language does not have bitwise ops
class AdHocOptimization : public Pass {
  public:
    AdHocOptimization(Module *m_) : Pass(m_) {
        m1 = std::make_unique<Module>("temp_mod");
        m = m1.get();
    }
    void run() override {
        loop_info = std::make_unique<LoopInfo>(m_);
        loop_info->run();
        // xor_pattern();
        // check_xor();
        // or_pattern();
        // check_or();
        // and_pattern();
        // check_and();
        check_memset();
        check_memcpy();
        check_median();
    }

  private:
    std::shared_ptr<Value> lookup;
    std::shared_ptr<Value> arg0;  // arg0 as of function `or`
    std::shared_ptr<Value> arg1;

    std::unique_ptr<Module> m1;
    Module *m;
    std::shared_ptr<Function> and_func;
    std::shared_ptr<Function> or_func;
    std::shared_ptr<Function> xor_func;
    // all four values are from original module, not the temporary one here
    BasicBlock *retbb, *valbb;
    Value *retval;
    Value *val_to_replace;
    std::unique_ptr<LoopInfo> loop_info;
    std::vector<std::pair<Loop*, StoreInst*>> memset_replace;
    void check_memcpy();
    void check_memset();
    void check_median();
    void check_and();
    void check_xor();
    void check_or() {
        for (auto f : m_->get_functions()) {
            if (f->get_num_of_args() != or_func->get_num_of_args())
                continue;
            for (auto bb : f->get_basic_blocks()) {
                eq_map.clear();
                auto f1_arg_it = f->get_args().begin();
                auto f2_arg_it = or_func->get_args().begin();
                for (size_t i = 0; i < or_func->get_num_of_args(); i++, f1_arg_it++, f2_arg_it++) {
                    eq_map[f1_arg_it->get()] = f2_arg_it->get();
                    if (i == 1)
                        arg0 = *f1_arg_it;
                    if (i == 2)
                        arg1 = *f1_arg_it;
                }
                val_to_replace = retval = retbb = nullptr;
                if (bb_eq(bb.get(), or_func->get_entry_block().get())) {
                    LOG_INFO << "found unoptimized or, replace " << val_to_replace->get_name();
                    optimized_or();
                }
            }
        }
    }
    void xor_pattern();
    void or_pattern() {
        // func
        or_func = Function::create(
            FunctionType::get(m->get_void_type(), {m->get_int32_type(), m->get_int32_type(), m->get_int32_type()}),
            "or_func",
            m);
        auto arg0 = *or_func->get_args().begin();
        auto arg1 = *++or_func->get_args().begin();
        auto arg2 = *++ ++or_func->get_args().begin();
        // bbs
        auto before_loop = BasicBlock::create(m, "beforeloop", or_func.get());
        auto preheader = BasicBlock::create(m, "preheader", or_func.get());
        auto body = BasicBlock::create(m, "body", or_func.get());
        auto out33 = BasicBlock::create(m, "out33", or_func.get());
        auto out37 = BasicBlock::create(m, "out37", or_func.get());
        auto then = BasicBlock::create(m, "then", or_func.get());
        auto else36 = BasicBlock::create(m, "else36", or_func.get());
        auto lor = BasicBlock::create(m, "lor", or_func.get());
        auto latch = BasicBlock::create(m, "latch", or_func.get());
        auto ret = BasicBlock::create(m, "return", or_func.get());
        // instructions, before_loop
        auto icmp_bl =
            CmpInst::create_cmp(CmpOp::LT, ConstantInt::get(1, m), ConstantInt::get(1073741824, m), before_loop.get());
        auto br_bl = BranchInst::create_cond_br(icmp_bl, preheader, out33, before_loop.get());
        // instructions, preheader
        auto br_preheader = BranchInst::create_br(body, preheader.get());
        // instructions, body
        auto phi1_body = PhiInst::create_phi(m->get_int32_type(), body.get());
        phi1_body->add_phi_pair_operand(std::static_pointer_cast<Value>(ConstantInt::get(1, m)), preheader);
        auto phi2_body = PhiInst::create_phi(m->get_int32_type(), body.get());
        phi2_body->add_phi_pair_operand(std::static_pointer_cast<Value>(ConstantInt::get(0, m)), preheader);
        // phi2_body->add_phi_pair_operand(phi_13_op5,latch);
        auto sdiv_body = BinaryInst::create_sdiv(arg1, phi1_body, body.get());
        auto srem_body = BinaryInst::create_srem(sdiv_body, ConstantInt::get(2, m), body.get());
        auto icmp_body = CmpInst::create_cmp(CmpOp::EQ, srem_body, ConstantInt::get(1, m), body.get());
        auto br_body = BranchInst::create_cond_br(icmp_body, then, lor, body.get());
        // instructions, out33
        auto phi_18_op6 = PhiInst::create_phi(m->get_int32_type(), out33.get());
        phi_18_op6->add_phi_pair_operand(std::static_pointer_cast<Value>(ConstantInt::get(1, m)), before_loop);
        auto phi_15_op5 = PhiInst::create_phi(m->get_int32_type(), out33.get());
        phi_15_op5->add_phi_pair_operand(std::static_pointer_cast<Value>(ConstantInt::get(0, m)), before_loop);
        auto br_out33 = BranchInst::create_br(ret, out33.get());
        // instructions, lor
        auto sdiv_lor = BinaryInst::create_sdiv(arg2, phi1_body, lor.get());
        auto srem_lor = BinaryInst::create_srem(sdiv_lor, ConstantInt::get(2, m), lor.get());
        auto icmp_lor = CmpInst::create_cmp(CmpOp::EQ, srem_lor, ConstantInt::get(1, m), lor.get());
        auto br_lor = BranchInst::create_cond_br(icmp_lor, then, else36, lor.get());
        // instructions, then
        auto mul_then = BinaryInst::create_mul(phi2_body, ConstantInt::get(2, m), then.get());
        std::shared_ptr<Value> add_then = BinaryInst::create_add(mul_then, ConstantInt::get(1, m), then.get());
        auto br_then = BranchInst::create_br(out37, then.get());
        // instructions, else
        std::shared_ptr<Value> mul_else = BinaryInst::create_mul(phi2_body, ConstantInt::get(2, m), else36.get());
        auto br_else = BranchInst::create_br(out37, else36.get());
        // instructions, out37
        auto phi_13_op5 = PhiInst::create_phi(m->get_int32_type(), out37.get());
        phi_13_op5->add_phi_pair_operand(add_then, then);
        phi_13_op5->add_phi_pair_operand(mul_else, else36);
        auto mul_out37 = BinaryInst::create_mul(phi1_body, ConstantInt::get(2, m), out37.get());
        auto br_out37 = BranchInst::create_br(latch, out37.get());
        // update phi
        phi_18_op6->add_phi_pair_operand(std::weak_ptr(mul_out37), latch);
        phi_15_op5->add_phi_pair_operand(std::weak_ptr(phi_13_op5), latch);
        phi1_body->add_phi_pair_operand(std::weak_ptr(mul_out37), latch);
        phi2_body->add_phi_pair_operand(std::weak_ptr(phi_13_op5), latch);
        // instructions, latch
        auto icmp_latch = CmpInst::create_cmp(CmpOp::LT, mul_out37, ConstantInt::get(1073741824, m), latch.get());
        auto br_latch = BranchInst::create_cond_br(icmp_latch, body, out33, latch.get());
        std::cout << or_func->print();
    }
    void and_pattern();
    std::map<Value *, Value *> eq_map;
    // compare for all instructions in bbs, branch targets and return value
    bool bb_eq(BasicBlock *lhs, BasicBlock *rhs) {
        if (lhs->get_name() == "label_return" and rhs->get_name() == "label_return")  // FIXME: easily broken
            return true;
        if (eq_map[lhs]) {
            if (eq_map[lhs] != rhs)
                return false;
            else  // checked
                return true;
        }
        auto &lhs_list = lhs->get_instructions();
        auto &rhs_list = rhs->get_instructions();
        if (lhs_list.size() != rhs_list.size())
            return false;
        eq_map[lhs] = rhs;
        auto lhs_it = lhs_list.begin();
        auto rhs_it = rhs_list.begin();
        for (size_t i = 0; i < lhs_list.size(); i++, lhs_it++, rhs_it++) {
            if (not inst_eq(lhs_it->get(), rhs_it->get()))
                return false;
        }
        return true;
    }
    bool inst_eq(Instruction *lhs, Instruction *rhs) {
        if (match(lhs, m_br(m_bb(retbb))) and retbb->get_terminator()->is_ret()) {
            retval = retbb->get_terminator()->get_operand(0).get();
            auto cur_bb = valbb = lhs->get_parent();
            if (isa<PhiInst>(retval))
                val_to_replace = dynamic_cast<PhiInst *>(retval)->input_of(cur_bb).get();
        }
        if (eq_map[lhs]) {
            if (eq_map[lhs] != rhs)
                return false;
            else  // checked
                return true;
        }
        if (lhs->get_instr_type() != rhs->get_instr_type())
            return false;
        eq_map[lhs] = rhs;
        auto &lhs_ops = lhs->get_operands();
        auto &rhs_ops = rhs->get_operands();
        if (lhs_ops.size() != rhs_ops.size())
            return false;
        for (size_t i = 0; i < lhs_ops.size(); i++) {
            auto lhs_op = lhs_ops[i].get();
            auto rhs_op = rhs_ops[i].get();
            if (eq_map[lhs_op] == rhs_op)
                continue;
            if (isa<ConstantInt>(lhs_op) or isa<ConstantInt>(rhs_op)) {
                auto lhs_const = dynamic_cast<ConstantInt *>(lhs_op);
                auto rhs_const = dynamic_cast<ConstantInt *>(rhs_op);
                return lhs_const and rhs_const and lhs_const->get_value() == rhs_const->get_value();
            }
            auto lhs_inst = dynamic_cast<Instruction *>(lhs_op);
            auto rhs_inst = dynamic_cast<Instruction *>(rhs_op);
            auto lhs_bb = dynamic_cast<BasicBlock *>(lhs_op);
            auto rhs_bb = dynamic_cast<BasicBlock *>(rhs_op);
            if ((lhs_inst or rhs_inst) and inst_eq(lhs_inst, rhs_inst))
                continue;
            if ((lhs_bb or rhs_bb) and bb_eq(lhs_bb, rhs_bb))
                continue;
            return false;
        }
        return true;
    }
    void optimized_or();
    void optimized_and();
    void optimized_xor();
};
