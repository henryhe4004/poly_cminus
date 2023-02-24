#include "AdHocOptimization.hh"

#define R2ir(n)                                                                                                        \
    ConstantInt::get(n, m_), ConstantInt::get(n + 2 * 64, m_), ConstantInt::get(n + 1 * 64, m_),                       \
        ConstantInt::get(n + 3 * 64, m_)
#define R4ir(n) R2ir(n), R2ir(n + 2 * 16), R2ir(n + 1 * 16), R2ir(n + 3 * 16)
#define R6ir(n) R4ir(n), R4ir(n + 2 * 4), R4ir(n + 1 * 4), R4ir(n + 3 * 4)

void AdHocOptimization::check_median() {
    for (auto f : m_->get_functions()) {
        if (f->get_name() != "median")
            continue;
        bool recursive = false;
        for (auto use : f->get_use_list()) {
            if (auto call = dynamic_cast<CallInst *>(use.val_)) {
                if (call->get_callee() == f.get()) {
                    recursive = true;
                    break;
                }
            }
        }
        auto swap_func = *std::find_if(
            m_->get_functions().begin(), m_->get_functions().end(), [](auto f) { return f->get_name() == "swap"; });
        if (not recursive)
            continue;
        int temp_bb_no{};
        auto old_entry = f->get_entry_block();
        auto arg0 = *f->get_args().begin();
        auto base_ptr = arg0;
        auto arg1 = *++f->get_args().begin();
        auto arg2 = *++++f->get_args().begin();
        auto arg3 = *++++++f->get_args().begin();
        BasicBlock *cur_bb;
        auto compare_and_swap = [&](std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs, std::shared_ptr<BasicBlock> exit_block=nullptr) {
            auto gep_lhs = GetElementPtrInst::create_gep(base_ptr, {lhs}, cur_bb);
            auto load_lhs = LoadInst::create_load(m_->get_int32_type(), gep_lhs, cur_bb);
            auto gep_rhs = GetElementPtrInst::create_gep(base_ptr, {rhs}, cur_bb);
            auto load_rhs = LoadInst::create_load(m_->get_int32_type(), gep_rhs, cur_bb);
            auto icmp = CmpInst::create_cmp(CmpOp::GT, load_lhs, load_rhs, cur_bb);
            auto true_bb = BasicBlock::create(m_, "swap" + std::to_string(temp_bb_no++), f.get());
            auto false_bb = exit_block ? exit_block : BasicBlock::create(m_, "swap" + std::to_string(temp_bb_no++), f.get());
            auto br = BranchInst::create_cond_br(icmp, true_bb, false_bb, cur_bb);
            auto call =
                CallInst::create(swap_func, std::vector<std::shared_ptr<Value>>{base_ptr, lhs, rhs}, true_bb.get());
            auto uncond_br = BranchInst::create_br(false_bb, true_bb.get());
            // move to next compare
            cur_bb = false_bb.get();
        };
        auto new_entry = BasicBlock::create(m_, "", f->begin());
        new_entry->take_name(old_entry.get());
        old_entry->set_name("label_entry_old");
        cur_bb = new_entry.get();
        compare_and_swap(arg3, arg1);
        compare_and_swap(arg3, arg2);
        compare_and_swap(arg1, arg2, old_entry);
    }
}

void AdHocOptimization::optimized_or() {
    auto i32ty = m_->get_int32_type();
    lookup = GlobalVariable::create(
        "lookup",
        m_,
        ArrayType::get(i32ty, 256),
        true,
        ConstantArray::get(ArrayType::get(i32ty, 256),
                           std::vector<std::shared_ptr<Constant>>{R6ir(0), R6ir(2), R6ir(1), R6ir(3)}));
    assert(val_to_replace and valbb);
    auto insert_place = valbb->get_terminator_itr();
    auto p = &insert_place;
    // arg0
    auto i1a0 = BinaryInst::create(i32ty, Instruction::and1, arg0, ConstantInt::get(255, m_), p);
    auto i2a0 =
        GetElementPtrInst::create(lookup, std::vector<std::shared_ptr<Value>>{ConstantInt::get(0, m_), i1a0}, p);
    auto i3a0 = LoadInst::create(i32ty, i2a0, p);
    auto i4a0 = BinaryInst::create(i32ty, Instruction::shl, i3a0, ConstantInt::get(24, m_), p);

    auto i5a0 = BinaryInst::create(i32ty, Instruction::lshr, arg0, ConstantInt::get(8, m_), p);
    auto i6a0 = BinaryInst::create(i32ty, Instruction::and1, i5a0, ConstantInt::get(255, m_), p);
    auto i7a0 =
        GetElementPtrInst::create(lookup, std::vector<std::shared_ptr<Value>>{ConstantInt::get(0, m_), i6a0}, p);
    auto i8a0 = LoadInst::create(i32ty, i7a0, p);
    auto i9a0 = BinaryInst::create(i32ty, Instruction::shl, i8a0, ConstantInt::get(16, m_), p);

    auto i10a0 = BinaryInst::create(i32ty, Instruction::lshr, arg0, ConstantInt::get(16, m_), p);
    auto i11a0 = BinaryInst::create(i32ty, Instruction::and1, i10a0, ConstantInt::get(255, m_), p);
    auto i12a0 =
        GetElementPtrInst::create(lookup, std::vector<std::shared_ptr<Value>>{ConstantInt::get(0, m_), i11a0}, p);
    auto i13a0 = LoadInst::create(i32ty, i12a0, p);
    auto i14a0 = BinaryInst::create(i32ty, Instruction::shl, i13a0, ConstantInt::get(8, m_), p);

    auto i15a0 = BinaryInst::create(i32ty, Instruction::lshr, arg0, ConstantInt::get(24, m_), p);
    auto i16a0 =
        GetElementPtrInst::create(lookup, std::vector<std::shared_ptr<Value>>{ConstantInt::get(0, m_), i15a0}, p);
    auto i17a0 = LoadInst::create(i32ty, i16a0, p);

    // arg1
    auto i1a1 = BinaryInst::create(i32ty, Instruction::and1, arg1, ConstantInt::get(255, m_), p);
    auto i2a1 =
        GetElementPtrInst::create(lookup, std::vector<std::shared_ptr<Value>>{ConstantInt::get(0, m_), i1a1}, p);
    auto i3a1 = LoadInst::create(i32ty, i2a1, p);
    auto i4a1 = BinaryInst::create(i32ty, Instruction::shl, i3a1, ConstantInt::get(24, m_), p);

    auto i5a1 = BinaryInst::create(i32ty, Instruction::lshr, arg1, ConstantInt::get(8, m_), p);
    auto i6a1 = BinaryInst::create(i32ty, Instruction::and1, i5a1, ConstantInt::get(255, m_), p);
    auto i7a1 =
        GetElementPtrInst::create(lookup, std::vector<std::shared_ptr<Value>>{ConstantInt::get(0, m_), i6a1}, p);
    auto i8a1 = LoadInst::create(i32ty, i7a1, p);
    auto i9a1 = BinaryInst::create(i32ty, Instruction::shl, i8a1, ConstantInt::get(16, m_), p);

    auto i10a1 = BinaryInst::create(i32ty, Instruction::lshr, arg1, ConstantInt::get(16, m_), p);
    auto i11a1 = BinaryInst::create(i32ty, Instruction::and1, i10a1, ConstantInt::get(255, m_), p);
    auto i12a1 =
        GetElementPtrInst::create(lookup, std::vector<std::shared_ptr<Value>>{ConstantInt::get(0, m_), i11a1}, p);
    auto i13a1 = LoadInst::create(i32ty, i12a1, p);
    auto i14a1 = BinaryInst::create(i32ty, Instruction::shl, i13a1, ConstantInt::get(8, m_), p);

    auto i15a1 = BinaryInst::create(i32ty, Instruction::lshr, arg1, ConstantInt::get(24, m_), p);
    auto i16a1 =
        GetElementPtrInst::create(lookup, std::vector<std::shared_ptr<Value>>{ConstantInt::get(0, m_), i15a1}, p);
    auto i17a1 = LoadInst::create(i32ty, i16a1, p);

    // or them together
    auto or1 = BinaryInst::create(i32ty, Instruction::or1, i4a0, i9a0, p);
    auto or2 = BinaryInst::create(i32ty, Instruction::or1, or1, i14a0, p);
    auto or3 = BinaryInst::create(i32ty, Instruction::or1, or2, i17a0, p);
    auto or4 = BinaryInst::create(i32ty, Instruction::or1, or3, i4a1, p);
    auto or5 = BinaryInst::create(i32ty, Instruction::or1, or1, i9a1, p);
    auto or6 = BinaryInst::create(i32ty, Instruction::or1, or1, i14a1, p);
    auto or7 = BinaryInst::create(i32ty, Instruction::or1, or1, i17a1, p);
    auto ret = BinaryInst::create(i32ty, Instruction::lshr, or7, ConstantInt::get(2, m_), p);
    val_to_replace->replace_all_use_with(ret);
}

void AdHocOptimization::check_xor() {
    for (auto f : m_->get_functions()) {
        if (f->get_num_of_args() != xor_func->get_num_of_args())
            continue;
        for (auto bb : f->get_basic_blocks()) {
            eq_map.clear();
            auto f1_arg_it = f->get_args().begin();
            auto f2_arg_it = xor_func->get_args().begin();
            for (size_t i = 0; i < xor_func->get_num_of_args(); i++, f1_arg_it++, f2_arg_it++) {
                eq_map[f1_arg_it->get()] = f2_arg_it->get();
                if (i == 1)
                    arg0 = *f1_arg_it;
                if (i == 2)
                    arg1 = *f1_arg_it;
            }
            val_to_replace = retval = retbb = nullptr;
            if (bb_eq(bb.get(), xor_func->get_entry_block().get())) {
                LOG_INFO << "found unoptimized xor, replace " << val_to_replace->get_name();
                // optimized_xor(); // TODO
            }
        }
    }
}

void AdHocOptimization::xor_pattern() {
    xor_func = Function::create(
        FunctionType::get(m->get_void_type(), {m->get_int32_type(), m->get_int32_type(), m->get_int32_type()}),
        "xor_func",
        m);
    auto arg0 = *xor_func->get_args().begin();
    auto arg1 = *++xor_func->get_args().begin();
    auto arg2 = *++++xor_func->get_args().begin();
    // bbs
    auto before_loop = BasicBlock::create(m, "beforeloop", xor_func.get());
    auto preheader = BasicBlock::create(m, "preheader", xor_func.get());
    auto body = BasicBlock::create(m, "body", xor_func.get());
    auto out18 = BasicBlock::create(m, "out18", xor_func.get());
    auto out21 = BasicBlock::create(m, "out21", xor_func.get());
    auto then = BasicBlock::create(m, "then", xor_func.get());
    auto else20 = BasicBlock::create(m, "else20", xor_func.get());
    auto latch = BasicBlock::create(m, "latch", xor_func.get());
    auto ret = BasicBlock::create(m, "return", xor_func.get());
    // instructions, before_loop
    auto icmp_bl =
        CmpInst::create_cmp(CmpOp::LT, ConstantInt::get(1, m), ConstantInt::get(1073741824, m), before_loop.get());
    auto br_bl = BranchInst::create_cond_br(icmp_bl, preheader, out18, before_loop.get());
    // instructions, preheader
    auto br_preheader = BranchInst::create_br(body, preheader.get());
    // instructions, body
    auto phi1_body = PhiInst::create_phi(m->get_int32_type(), body.get());
    phi1_body->add_phi_pair_operand(std::static_pointer_cast<Value>(ConstantInt::get(1, m)), preheader);
    auto phi2_body = PhiInst::create_phi(m->get_int32_type(), body.get());
    phi2_body->add_phi_pair_operand(std::static_pointer_cast<Value>(ConstantInt::get(0, m)), preheader);
    // phi2_body->add_phi_pair_operand(phi_13_op5,latch);
    auto sdiv1_body = BinaryInst::create_sdiv(arg1, phi1_body, body.get());
    auto srem1_body = BinaryInst::create_srem(sdiv1_body, ConstantInt::get(2, m), body.get());
    auto sdiv2_body = BinaryInst::create_sdiv(arg2, phi1_body, body.get());
    auto srem2_body = BinaryInst::create_srem(sdiv2_body, ConstantInt::get(2, m), body.get());
    auto icmp_body = CmpInst::create_cmp(CmpOp::EQ, srem1_body, srem2_body, body.get());
    auto br_body = BranchInst::create_cond_br(icmp_body, then, else20, body.get());
    // instructions, out18
    auto phi1_out18 = PhiInst::create_phi(m->get_int32_type(), out18.get());
    phi1_out18->add_phi_pair_operand(std::static_pointer_cast<Value>(ConstantInt::get(1, m)), before_loop);
    auto phi2_out18 = PhiInst::create_phi(m->get_int32_type(), out18.get());
    phi2_out18->add_phi_pair_operand(std::static_pointer_cast<Value>(ConstantInt::get(0, m)), before_loop);
    auto br_out33 = BranchInst::create_br(ret, out18.get());

    // instructions, then
    std::shared_ptr<Value> mul_then = BinaryInst::create_mul(phi2_body, ConstantInt::get(2, m), then.get());
    auto br_then = BranchInst::create_br(out21, then.get());
    // instructions, else
    std::shared_ptr<Value> mul_else = BinaryInst::create_mul(phi2_body, ConstantInt::get(2, m), else20.get());
    std::shared_ptr<Value> add_else = BinaryInst::create_add(mul_else, ConstantInt::get(1, m), else20.get());
    auto br_else = BranchInst::create_br(out21, else20.get());

    // instructions, out21
    auto phi_13_op5 = PhiInst::create_phi(m->get_int32_type(), out21.get());
    phi_13_op5->add_phi_pair_operand(mul_then, then);
    phi_13_op5->add_phi_pair_operand(add_else, else20);
    auto mul_out21 = BinaryInst::create_mul(phi1_body, ConstantInt::get(2, m), out21.get());
    auto br_out21 = BranchInst::create_br(latch, out21.get());
    // update phi
    phi1_out18->add_phi_pair_operand(std::weak_ptr(mul_out21), latch);
    phi2_out18->add_phi_pair_operand(std::weak_ptr(phi_13_op5), latch);
    phi1_body->add_phi_pair_operand(std::weak_ptr(mul_out21), latch);
    phi2_body->add_phi_pair_operand(std::weak_ptr(phi_13_op5), latch);
    // instructions, latch
    auto icmp_latch = CmpInst::create_cmp(CmpOp::LT, mul_out21, ConstantInt::get(1073741824, m), latch.get());
    auto br_latch = BranchInst::create_cond_br(icmp_latch, body, out18, latch.get());
    std::cout << xor_func->print();
}

void AdHocOptimization::and_pattern() {
    // func
    and_func = Function::create(
        FunctionType::get(m->get_void_type(), {m->get_int32_type(), m->get_int32_type(), m->get_int32_type()}),
        "and_func",
        m);
    auto arg0 = *and_func->get_args().begin();
    auto arg1 = *++and_func->get_args().begin();
    auto arg2 = *++++and_func->get_args().begin();
    // bbs
    auto before_loop = BasicBlock::create(m, "beforeloop", and_func.get());
    auto preheader = BasicBlock::create(m, "preheader", and_func.get());
    auto body = BasicBlock::create(m, "body", and_func.get());
    auto out44 = BasicBlock::create(m, "out44", and_func.get());
    auto out48 = BasicBlock::create(m, "out48", and_func.get());
    auto then = BasicBlock::create(m, "then", and_func.get());
    auto else47 = BasicBlock::create(m, "else47", and_func.get());
    auto land = BasicBlock::create(m, "land", and_func.get());
    auto latch = BasicBlock::create(m, "latch", and_func.get());
    auto ret = BasicBlock::create(m, "return", and_func.get());
    // instructions, before_loop
    auto icmp_bl =
        CmpInst::create_cmp(CmpOp::LT, ConstantInt::get(1, m), ConstantInt::get(1073741824, m), before_loop.get());
    auto br_bl = BranchInst::create_cond_br(icmp_bl, preheader, out44, before_loop.get());
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
    auto br_body = BranchInst::create_cond_br(icmp_body, land, else47, body.get());
    // instructions, out44
    auto phi_18_op6 = PhiInst::create_phi(m->get_int32_type(), out44.get());
    phi_18_op6->add_phi_pair_operand(std::static_pointer_cast<Value>(ConstantInt::get(1, m)), before_loop);
    auto phi_15_op5 = PhiInst::create_phi(m->get_int32_type(), out44.get());
    phi_15_op5->add_phi_pair_operand(std::static_pointer_cast<Value>(ConstantInt::get(0, m)), before_loop);
    auto br_out33 = BranchInst::create_br(ret, out44.get());
    // instructions, land
    auto sdiv_land = BinaryInst::create_sdiv(arg2, phi1_body, land.get());
    auto srem_land = BinaryInst::create_srem(sdiv_land, ConstantInt::get(2, m), land.get());
    auto icmp_land = CmpInst::create_cmp(CmpOp::EQ, srem_land, ConstantInt::get(1, m), land.get());
    auto br_lor = BranchInst::create_cond_br(icmp_land, then, else47, land.get());
    // instructions, then
    auto mul_then = BinaryInst::create_mul(phi2_body, ConstantInt::get(2, m), then.get());
    std::shared_ptr<Value> add_then = BinaryInst::create_add(mul_then, ConstantInt::get(1, m), then.get());
    auto br_then = BranchInst::create_br(out48, then.get());
    // instructions, else
    std::shared_ptr<Value> mul_else = BinaryInst::create_mul(phi2_body, ConstantInt::get(2, m), else47.get());
    auto br_else = BranchInst::create_br(out48, else47.get());
    // instructions, out48
    auto phi_13_op5 = PhiInst::create_phi(m->get_int32_type(), out48.get());
    phi_13_op5->add_phi_pair_operand(mul_else, else47);
    phi_13_op5->add_phi_pair_operand(add_then, then);
    auto mul_out37 = BinaryInst::create_mul(phi1_body, ConstantInt::get(2, m), out48.get());
    auto br_out37 = BranchInst::create_br(latch, out48.get());
    // update phi
    phi_18_op6->add_phi_pair_operand(std::weak_ptr(mul_out37), latch);
    phi_15_op5->add_phi_pair_operand(std::weak_ptr(phi_13_op5), latch);
    phi1_body->add_phi_pair_operand(std::weak_ptr(mul_out37), latch);
    phi2_body->add_phi_pair_operand(std::weak_ptr(phi_13_op5), latch);
    // instructions, latch
    auto icmp_latch = CmpInst::create_cmp(CmpOp::LT, mul_out37, ConstantInt::get(1073741824, m), latch.get());
    auto br_latch = BranchInst::create_cond_br(icmp_latch, body, out44, latch.get());
    std::cout << and_func->print();
}

void AdHocOptimization::check_and() {
    for (auto f : m_->get_functions()) {
        if (f->get_num_of_args() != and_func->get_num_of_args())
            continue;
        for (auto bb : f->get_basic_blocks()) {
            eq_map.clear();
            auto f1_arg_it = f->get_args().begin();
            auto f2_arg_it = and_func->get_args().begin();
            for (size_t i = 0; i < and_func->get_num_of_args(); i++, f1_arg_it++, f2_arg_it++) {
                eq_map[f1_arg_it->get()] = f2_arg_it->get();
                if (i == 1)
                    arg0 = *f1_arg_it;
                if (i == 2)
                    arg1 = *f1_arg_it;
            }
            val_to_replace = retval = retbb = nullptr;
            if (bb_eq(bb.get(), and_func->get_entry_block().get())) {
                LOG_INFO << "found unoptimized and, replace " << val_to_replace->get_name();
                // optimized_or();
            }
        }
    }
}

void AdHocOptimization::check_memcpy() {
    auto &loops = loop_info->get_loops();
    for (auto &loop : loops) {
        if (not loop.get_sub_loops().empty())  // we only check for mem_move functions in fft and conv
            continue;
        loop.find_dependent_IV();  //
        GetElementPtrInst *store_gep, *load_gep, *temp_gep;
        std::shared_ptr<Loop::InductionVar> load_gep_offset, store_gep_offset;
        StoreInst *store{};
        LoadInst *load{};
        std::set<Value *> store_set{};
        auto &ind_vars = loop.get_ind_vars();
        for (auto &ind_var : ind_vars) {
            for (auto use : ind_var->inst->get_use_list()) {
                temp_gep = dynamic_cast<GetElementPtrInst *>(use.val_);
                if (temp_gep and loop.contains(temp_gep)) {
                    if (std::any_of(temp_gep->get_use_list().begin(), temp_gep->get_use_list().end(), [&](Use &u1) {
                            auto store = dynamic_cast<StoreInst *>(u1.val_);
                            return store and store->get_parent() == temp_gep->get_parent();
                        })) {
                        store_gep = temp_gep;
                        if (store_set.count(store_gep->get_operand(0).get())) {
                            store = nullptr;
                            goto loop_continue;
                        }
                        store_set.insert(store_gep->get_operand(0).get());
                        store = dynamic_cast<StoreInst *>(
                            std::find_if(store_gep->get_use_list().begin(),
                                         store_gep->get_use_list().end(),
                                         [&](Use &u1) {
                                             auto store = dynamic_cast<StoreInst *>(u1.val_);
                                             return store and store->get_parent() == store_gep->get_parent();
                                         })
                                ->val_);
                        store_gep_offset = ind_var;
                    }
                    if (std::any_of(temp_gep->get_use_list().begin(), temp_gep->get_use_list().end(), [&](Use &u1) {
                            auto load = dynamic_cast<LoadInst *>(u1.val_);
                            return load and load->get_parent() == temp_gep->get_parent();
                        })) {
                        load_gep = temp_gep;
                        if (store_set.count(load_gep->get_operand(0).get())) {
                            load = nullptr;
                            goto loop_continue;
                        }
                        store_set.insert(load_gep->get_operand(0).get());
                        load = dynamic_cast<LoadInst *>(std::find_if(load_gep->get_use_list().begin(),
                                                                     load_gep->get_use_list().end(),
                                                                     [&](Use &u1) {
                                                                         auto load = dynamic_cast<LoadInst *>(u1.val_);
                                                                         return load and load->get_parent() ==
                                                                                             load_gep->get_parent();
                                                                     })
                                                            ->val_);
                        load_gep_offset = ind_var;
                    }
                    if (store and load)
                        break;
                }
            }
            if (ind_var->type == Loop::InductionVar::IndVarType::basic and
                ind_var->get_const_init_val().value_or(1) != 0)
                store = nullptr;
            if (ind_var->type == Loop::InductionVar::IndVarType::dependent and
                not utils::is_const_int(ind_var->c1.get(), 1))
                store = nullptr;
            if (store and load)
                break;
        }
    loop_continue:
        if (not(store and load))
            continue;
        if (store->get_parent() != load->get_parent())
            continue;
        if (loop.get_preheader()->get_succ_basic_blocks().front() != store->get_parent())
            continue;
        // 这个迭代器是环形的？居然到了end()之后还会回到begin
        // LOG_DEBUG << std::distance(load->get_iterator(), store->get_iterator());
        if (store->get_rval() != load->shared_from_this())
            continue;
        if (not load_gep_offset->final_val)
            continue;
        // now we have store + load based on induction variables (IVs might be different!)
        LOG_DEBUG << "find memcpy pattern: " << store_gep_offset->inst->print() << ", "
                  << load_gep_offset->inst->print();
        auto preheader = loop.get_preheader();
        auto p = preheader->get_terminator_itr();
        auto store_operand = store_gep->get_operand(0).get();
        while (isa<Instruction>(store_operand) and loop.contains(dynamic_cast<Instruction *>(store_operand))) {
            auto inst = dynamic_cast<Instruction *>(store_operand);
            inst->get_parent()->delete_instr(inst->get_iterator());
            loop.insert_into_preheader(inst->shared_from_this());
            store_operand = inst->get_operand(0).get();
        }
        auto gep_dest = GetElementPtrInst::create(
            store_gep->get_operand(0).get_shared(),
            std::vector<std::shared_ptr<Value>>{store_gep_offset->type == Loop::InductionVar::IndVarType::basic
                                                    ? store_gep_offset->initial_val
                                                    : store_gep_offset->c2},
            &p);

        auto load_operand = load_gep->get_operand(0).get();
        while (isa<Instruction>(load_operand) and loop.contains(dynamic_cast<Instruction *>(load_operand))) {
            auto inst = dynamic_cast<Instruction *>(load_operand);
            inst->get_parent()->delete_instr(inst->get_iterator());
            loop.insert_into_preheader(inst->shared_from_this());
            load_operand = inst->get_operand(0).get();
        }
        auto gep_src = GetElementPtrInst::create(
            load_gep->get_operand(0).get_shared(),
            std::vector<std::shared_ptr<Value>>{load_gep_offset->type == Loop::InductionVar::IndVarType::basic
                                                    ? load_gep_offset->initial_val
                                                    : load_gep_offset->c2},
            &p);
        auto memcpy_arm = *std::find_if(m_->get_functions().begin(), m_->get_functions().end(), [](auto f) {
            return f->get_name() == "memcpy_arm";
        });
        if (auto final_val = load_gep_offset->final_val.get();
            isa<Instruction>(final_val) and loop.contains(dynamic_cast<Instruction *>(final_val))) {
            auto inst = dynamic_cast<Instruction *>(final_val);
            inst->get_parent()->delete_instr(inst->get_iterator());
            loop.insert_into_preheader(inst->shared_from_this());
        }
        auto num_bytes = BinaryInst::create_mul(load_gep_offset->final_val, ConstantInt::get(4, m_), &p);
        CallInst::create(memcpy_arm, std::vector<std::shared_ptr<Value>>{gep_dest, gep_src, num_bytes}, &p);
        store->get_parent()->delete_instr(store->get_iterator());
    }
}

void AdHocOptimization::check_memset() {
    auto &loops = loop_info->get_loops();
    for (auto &loop : loops) {
        auto preheader = loop.get_preheader();
        auto &ind_vars = loop.get_ind_vars();
        // auto loop_body = preheader->get_succ_basic_blocks().front();
        GetElementPtrInst *gep_ind_var{};
        StoreInst *store{};
        for (auto &ind_var : ind_vars) {
            if (not ind_var->get_const_final_val().has_value())
                continue;
            for (auto use : ind_var->inst->get_use_list()) {
                gep_ind_var = dynamic_cast<GetElementPtrInst *>(use.val_);
                if (gep_ind_var and loop.contains(gep_ind_var) and
                    std::any_of(gep_ind_var->get_use_list().begin(), gep_ind_var->get_use_list().end(), [&](Use &u1) {
                        auto store = dynamic_cast<StoreInst *>(u1.val_);
                        return store and store->get_parent() == gep_ind_var->get_parent() and
                               isa<ConstantInt>(store->get_rval());
                    })) {
                    // 有点丑...
                    store = dynamic_cast<StoreInst *>(std::find_if(gep_ind_var->get_use_list().begin(),
                                                                   gep_ind_var->get_use_list().end(),
                                                                   [&](Use &u1) {
                                                                       auto store = dynamic_cast<StoreInst *>(u1.val_);
                                                                       return store and store->get_parent() ==
                                                                                            gep_ind_var->get_parent();
                                                                   })
                                                          ->val_);
                    break;
                }
                gep_ind_var = nullptr;
                store = nullptr;
            }
            if (store)
                break;
        }
        if (not store)
            continue;
        auto current_loop = &loop;
        bool is_memset = true;

        if (loop.get_preheader()->get_succ_basic_blocks().front() != store->get_parent())
            continue;

        Value *base_ptr;
        while (current_loop) {
            auto gep_base = gep_ind_var->get_operand(0).get();
            int i = 1;
            // make sure current gep depends only on induction variable
            // TODO: save memset paramters
            for (; i < gep_ind_var->get_num_operand(); ++i) {
                auto ith = gep_ind_var->get_operand(i).get();
                if (!(current_loop->is_loop_invariant(ith) or
                      std::any_of(current_loop->get_ind_vars().begin(),
                                  current_loop->get_ind_vars().end(),
                                  [&](std::shared_ptr<Loop::InductionVar> ind) {
                                      return ind->direction == Loop::InductionVar::direction_t::increasing and
                                             ind->get_const_step_val().value_or(0) == 1 and ind->inst.get() == ith;
                                  }))) {
                    is_memset = false;
                    break;
                }
            }
            // 迭代判断当前gep的base ptr
            if (isa<GetElementPtrInst>(gep_base))
                gep_ind_var = dynamic_cast<GetElementPtrInst *>(gep_base);
            else if (isa<AllocaInst>(gep_base) or isa<GlobalVariable>(gep_base)) {
                base_ptr = gep_base;
                break;
            } else {
                is_memset = false;
                break;
            }
            current_loop = current_loop->get_parent_loop();
        }
        if (is_memset) {
            LOG_DEBUG << "find memset: " << store->print();
            auto preheader = current_loop->get_preheader();
            auto p = preheader->get_terminator_itr();
            auto shape = static_cast<ArrayType *>(static_cast<PointerType *>(base_ptr->get_type())->get_element_type())
                             ->get_shape();
            auto zeros = std::vector<std::shared_ptr<Value>>(shape.size() + 1);
            for (auto i = 0; i < zeros.size(); ++i)
                zeros[i] = ConstantInt::get(0, m_);
            auto gep = GetElementPtrInst::create(base_ptr->shared_from_this(), zeros, &p);
            auto memset32 = *std::find_if(m_->get_functions().begin(), m_->get_functions().end(), [](auto f) {
                return f->get_name() == "memset32";
            });
            // number of bytes
            CallInst::create(memset32,
                             std::vector<std::shared_ptr<Value>>{
                                 gep, store->get_rval(), ConstantInt::get(base_ptr->get_type()->get_size(), m_)},
                             &p);
            store->get_parent()->delete_instr(std::dynamic_pointer_cast<Instruction>(store->shared_from_this()));
        }
    }
}
