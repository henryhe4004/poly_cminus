#include "LoopInfo.hh"

void LoopInfo::run() {
    loop_search_.run();
    // std::cout << m_->print();
    for (auto l_s : loop_search_) {
        // loops.emplace(loops.end(), *l);
        Loop l(*l_s, m_);
        l.base = std::static_pointer_cast<BasicBlock>(loop_search_.get_loop_base(l_s)->shared_from_this());
        l.find_pre_header_and_end();
        l.find_latch();
        l.find_break();
        // l.find_all_IV();
        l.find_basic_IV();
        l.find_bound();
        bbset_ptr_to_loop_idx.emplace(l_s, loops.size());
        loops.push_back(l);
    }
    for (auto l_s : loop_search_) {
        auto cur_loop_ptr = &loops[bbset_ptr_to_loop_idx[l_s]];
        auto parent_loop_set = loop_search_.get_parent_loop(l_s);
        if (parent_loop_set) {
            loops[bbset_ptr_to_loop_idx[parent_loop_set]].sub_loops.push_back(cur_loop_ptr);
            cur_loop_ptr->parent_loop = &loops[bbset_ptr_to_loop_idx[parent_loop_set]];
        }
    }
    test_print();
}

void LoopInfo::test_print() {
    auto count = 0;
    std::string ident = "";
    for (auto &l : loops) {
        using std::cout;
        using std::endl;
        cout << ident << "loop " << std::to_string(count++) << " in func " << l.base->get_parent()->get_name() << endl;
        ident.push_back(' ');
        cout << ident << "blocks:\n";
        ident.push_back(' ');
        for (auto B : l.get_blocks()) {
            cout << ident << B->get_name() << endl;
        }
        ident.pop_back();
        cout << ident << "base: " << l.get_base()->get_name() << endl;
        if (l.parent_loop)
            cout << ident << "parent base: " << l.get_parent_loop()->get_base()->get_name() << endl;
        cout << ident << "sub loops: \n";
        ident.push_back(' ');
        for (auto sub : l.get_sub_loops()) {
            cout << ident << sub->get_base()->get_name() << endl;
        }
        ident.pop_back();
        cout << ident << "preheader: " << l.get_preheader()->get_name() << endl;
        cout << ident << "end: " << l.get_end()->get_name() << endl;
        cout << ident << "latch: " << l.get_latch()->get_name() << endl;
        cout << ident << "exit: " << l.get_exit()->get_name() << endl;
        cout << ident << "has break statement: " << (l.has_break_etmt() ? "true\n" : "false\n");
        cout << ident << "induction variable(s):\n";
        ident.push_back(' ');
        for (auto iv : l.get_ind_vars()) {
            cout << ident << "IV: \n";
            ident.push_back(' ');
            cout << ident << "instruction: " << iv->inst->get_name() << endl;
            if (iv->type == ivtype::basic) {
                cout << ident << "type: basic\n";
                cout << ident << "step_inst: " << iv->step_inst->get_name() << endl;
                cout << ident << "bin_op: " << iv->step_inst->get_instr_op_name() << endl;
                cout << ident << "initial value: " << iv->initial_val->get_name() << endl;
                cout << ident << "step value: " << iv->step_val->get_name() << endl;
                if (iv->final_val)
                    cout << ident << "final value: " << iv->final_val->get_name() << endl;
                cout << ident << "direction: ";
                if (iv->direction == Loop::InductionVar::direction_t::decreasing)
                    cout << "decreasing" << endl;
                else if (iv->direction == Loop::InductionVar::direction_t::increasing)
                    cout << "increasing" << endl;
            } else {
                cout << ident << "type: dependent\n";
                cout << ident << "basis: " << iv->basis->inst->get_name() << endl;
                cout << ident << "c1: " << iv->c1->get_name() << endl;
                cout << ident << "c2: " << iv->c2->get_name() << endl;
            }
            if (auto ci = iv->get_const_init_val(); ci.has_value())
                cout << ident << "const initial value: " << ci.value() << endl;
            if (auto cs = iv->get_const_step_val(); cs.has_value())
                cout << ident << "const step value: " << cs.value() << endl;
            if (auto cf = iv->get_const_final_val(); cf.has_value())
                cout << ident << "const final value: " << cf.value() << endl;
            ident.pop_back();
        }
        if (l.get_bound_IV())
            cout << ident << "loop bound iv: " << l.get_bound_IV()->inst->get_name() << endl;
        ident.pop_back();
        ident.pop_back();
    }
}

void Loop::find_pre_header_and_end() {
    // auto &blocks = get_blocks();
    BasicBlock *pre = nullptr, *end = nullptr;
    for (auto base_pre : get_base()->get_pre_basic_blocks()) {
        if (blocks.find(base_pre) != blocks.end())
            end = base_pre;
        else
            pre = base_pre;
    }
    if (pre->get_terminator()->get_num_operand() == 1)
        this->preheader = std::static_pointer_cast<BasicBlock>(pre->shared_from_this());
    else
        this->preheader = nullptr;
    this->end_block = std::static_pointer_cast<BasicBlock>(end->shared_from_this());
    if (end) {
        if (end->get_succ_basic_blocks().size() != 2) {
            LOG_DEBUG << "The end block of the loop " << base->get_name()
                      << " does not have two successors (one to loop body, another to afterloop)";
            this->exit = nullptr;
            return;
        }
        auto end_succ = end->get_succ_basic_blocks().begin();
        if (contains(*end_succ))
            end_succ++;
        exit = std::static_pointer_cast<BasicBlock>((*end_succ)->shared_from_this());
    }
}

void Loop::find_basic_IV() {
    for (auto instr : base->get_instructions()) {
        /// 如果一个phi结点是基本归纳变量，那么它应该满足：
        /// 1.只提及两个前驱基本块
        /// 2.且这两个基本块一个在循环中，另一个不在
        /// 循环外来的值是归纳变量的初始值，循环中来的值是当前值增加一个步长后的值(step_inst)
        /// 该step_inst又应满足：
        /// 3.是加法或减法指令
        /// 4.其一个操作数为前述phi结点
        /// 5.另一个操作数为循环不变量（即步长）
        /// 6.被一条比较指令使用（边界）
        /// 6补：该比较指令的另一个操作数应为循环不变量
        /// 6改：不一定，此时终止值未知
        /// 为了识别循环不变量，这里要求已经进行过不变式外提

        // 条件1.
        if (!(instr->is_phi() && instr->get_num_operand() == 4))
            continue;
        Value *ind_initial = nullptr, *ind_changed = nullptr;
        for (int i : {1, 3}) {
            if (contains(static_cast<BasicBlock *>(instr->get_operand(i).get())))
                ind_changed = instr->get_operand(i - 1).get();
            else
                ind_initial = instr->get_operand(i - 1).get();
        }
        // 条件2.
        if (!ind_initial || !ind_changed)
            continue;
        auto step_instr = dynamic_cast<Instruction *>(ind_changed);
        // 条件3.
        if (!step_instr || !(step_instr->is_add() || step_instr->is_sub()))
            continue;
        // 条件4.
        Value *step;
        if (step_instr->get_operand(0).get() == instr.get())
            step = step_instr->get_operand(1).get();
        else if (step_instr->get_operand(1).get() == instr.get())
            step = step_instr->get_operand(0).get();
        else
            continue;
        // 条件5.
        if (!is_loop_invariant(step))
            continue;
        // 条件6.
        Value *final_val = nullptr;
        CmpInst *bound_inst = nullptr;
        // 仅能识别类似while (i < 100)这种形式的终止值，此时latch部分应只有一个基本块
        // 无法识别while (i < x && i < y)这样的
        if (latch == end_block) {
            for (auto use_of_step_inst : ind_changed->get_use_list()) {
                if (auto use_inst = dynamic_cast<Instruction *>(use_of_step_inst.val_)) {
                    if (use_inst->is_cmp() && use_inst->get_parent() == latch.get()) {
                        if (use_inst->get_operand(0).get() == ind_changed)
                            final_val = use_inst->get_operand(1).get();
                        else
                            final_val = use_inst->get_operand(0).get();
                        bound_inst = static_cast<CmpInst *>(use_inst);
                    }
                }
            }
        }
        if (!is_loop_invariant(final_val)) {
            final_val = nullptr;
            bound_inst = nullptr;
        }
        if (has_break_) {
            final_val = nullptr;
            bound_inst = nullptr;
        }
        // if (!final_val)
        //     continue;
        // InductionVar ind_var;
        auto ind_var = shared_ptr<InductionVar>(new InductionVar);
        ind_var->inst = instr;
        ind_var->type = InductionVar::IndVarType::basic;
        ind_var->initial_val = ind_initial->shared_from_this();
        ind_var->step_val = step->shared_from_this();
        ind_var->basis = nullptr;
        ind_var->bin_op = step_instr->get_instr_type();
        if (final_val)
            ind_var->final_val = final_val->shared_from_this();
        ind_var->step_inst = std::static_pointer_cast<Instruction>(step_instr->shared_from_this());
        if (bound_inst)
            ind_var->predicate = bound_inst->get_cmp_op();
        if (auto c_step = ind_var->get_const_step_val(); c_step.has_value()) {
            if (c_step.value() > 0) {
                if (step_instr->is_add())
                    ind_var->direction = InductionVar::direction_t::increasing;
                else
                    ind_var->direction = InductionVar::direction_t::decreasing;
            } else if (c_step.value() < 0) {
                if (step_instr->is_add())
                    ind_var->direction = InductionVar::direction_t::decreasing;
                else
                    ind_var->direction = InductionVar::direction_t::increasing;
            } else
                ind_var->direction = InductionVar::direction_t::unknown;
        } else
            ind_var->direction = InductionVar::direction_t::unknown;
        // ind_vars.push_back(ind_var);
        ind_vars.insert(ind_var);
        inst2iv.insert({instr.get(), ind_var});
    }
}

void Loop::find_dependent_IV() {
    // 识别循环中的依赖归纳变量
    // 一个依赖归纳变量指：
    // 设i是一个基本归纳变量，则称满足线性方程 j = b * i + c 的j为一个依赖归纳变量
    // i称为j的基
    bool add_new = true;
    while (add_new) {
        add_new = false;
        for (auto bb : get_blocks()) {
            for (auto I : bb->get_instructions()) {
                if (IV_exists(I.get()))
                    continue;
                if (I->is_phi())
                    continue;
                if (I->is_add() || I->is_sub() || I->is_mul()) {
                    ivptr iv = IV_exists(dynamic_cast<Instruction *>(I->get_operand(0).get()));
                    Value *para_val;  // 线性表达式的参数
                    int iv_pos;
                    if (iv) {
                        para_val = I->get_operand(1).get();
                        iv_pos = 0;
                    } else {
                        iv = IV_exists(dynamic_cast<Instruction *>(I->get_operand(1).get()));
                        para_val = I->get_operand(0).get();
                        iv_pos = 1;
                    }
                    if (iv == nullptr || !lazy_invariant(para_val))
                        continue;
                    // if (iv->type == ivtype::basic && I == iv->step_inst)
                    //     continue;
                    auto new_iv = ivptr(new InductionVar(I, ivtype::dependent));
                    if (iv->type == ivtype::basic) {
                        new_iv->basis = iv;
                        switch (I->get_instr_type()) {
                            case Instruction::add:
                                new_iv->c1 = ConstantInt::get(1, m_);
                                new_iv->c2 = para_val->shared_from_this();
                                // new_iv->c1_sign = InductionVar::sign_t::pos;
                                // new_iv->c2_sign = InductionVar::sign_t::pos;
                                break;
                            case Instruction::sub:
                                if (iv_pos == 0) {
                                    new_iv->c1 = ConstantInt::get(1, m_);
                                    new_iv->c2 =
                                        utils::sub_i32(ConstantInt::get(0, m_), para_val->shared_from_this(), m_);
                                    insert_into_preheader(new_iv->c2);
                                    // new_iv->c1_sign = InductionVar::sign_t::pos;
                                    // new_iv->c2_sign = InductionVar::sign_t::neg;
                                } else {
                                    new_iv->c1 = ConstantInt::get(-1, m_);
                                    new_iv->c2 = para_val->shared_from_this();
                                    // new_iv->c1_sign = InductionVar::sign_t::neg;
                                    // new_iv->c2_sign = InductionVar::sign_t::pos;
                                }
                                break;
                            case Instruction::mul:
                                new_iv->c1 = para_val->shared_from_this();
                                new_iv->c2 = ConstantInt::get(0, m_);
                                // new_iv->c1_sign = InductionVar::sign_t::pos;
                                // new_iv->c2_sign = InductionVar::sign_t::pos;
                                break;
                            default:
                                break;
                        }
                    } else {
                        new_iv->basis = iv->basis;
                        // auto new_iv_para_const = utils::get_const_int_val(para_val);
                        // auto iv_c1_const = utils::get_const_int_val(iv->c1.get());
                        // auto iv_c2_const = utils::get_const_int_val(iv->c2.get());
                        switch (I->get_instr_type()) {
                            case Instruction::add:
                                new_iv->c1 = iv->c1;
                                new_iv->c2 =
                                    utils::add_i32(para_val->shared_from_this(), iv->c2->shared_from_this(), m_);
                                insert_into_preheader(new_iv->c2);
                                break;
                            case Instruction::sub:
                                if (iv_pos == 0) {
                                    // j = (iv->c1 * i + iv->c2) - para
                                    new_iv->c1 = iv->c1;
                                    new_iv->c2 =
                                        utils::sub_i32(iv->c2->shared_from_this(), para_val->shared_from_this(), m_);
                                    insert_into_preheader(new_iv->c2);
                                } else {
                                    // j = para - (iv->c1 * i + iv->c2)
                                    new_iv->c1 =
                                        utils::sub_i32(ConstantInt::get(0, m_), iv->c1->shared_from_this(), m_);
                                    new_iv->c2 = utils::sub_i32(para_val->shared_from_this(), iv->c2, m_);
                                    insert_into_preheader(new_iv->c1);
                                    insert_into_preheader(new_iv->c2);
                                }
                                break;
                            case Instruction::mul:
                                // j = (iv->c1 * i + iv->c2) * para
                                new_iv->c1 = utils::mul_i32(iv->c1, para_val->shared_from_this(), m_);
                                new_iv->c2 = utils::mul_i32(iv->c2, para_val->shared_from_this(), m_);
                                insert_into_preheader(new_iv->c1);
                                insert_into_preheader(new_iv->c2);
                            default:
                                break;
                        }
                    }
                    inst2iv.insert({I.get(), new_iv});
                    ind_vars.insert(new_iv);
                }
            }
        }
    }
}

void Loop::find_latch() {
    auto lat = end_block.get();
    latch_blocks.insert(lat);
    while (true) {
        if (lat->get_pre_basic_blocks().size() == 1) {
            auto lat_pre = *(lat->get_pre_basic_blocks().begin());
            if (lat_pre->get_succ_basic_blocks().size() == 2) {
                BasicBlock *maybe_exit;
                auto suc_begin = lat_pre->get_succ_basic_blocks().begin();
                if ((*suc_begin) == lat)
                    suc_begin++;
                maybe_exit = *suc_begin;
                if (maybe_exit == exit.get()) {
                    lat = lat_pre;
                    latch_blocks.insert(lat);
                    continue;
                }
            }
        }
        break;
    }
    latch = std::static_pointer_cast<BasicBlock>(lat->shared_from_this());
    return;
}

bool Loop::is_loop_invariant(Value *v) {
    // if (LoopInfo::is_constant(v))
    //     return true;
    if (auto inst = dynamic_cast<Instruction *>(v)) {
        if (!contains(inst))
            return true;
        if (auto phi = dynamic_cast<PhiInst *>(inst)) {
            for (auto op : inst->get_operands()) {
                if (!lazy_invariant(op.get()))
                    return false;
            }
        } else {
            for (auto op : inst->get_operands()) {
                if (!is_loop_invariant(op.get()))
                    return false;
            }
        }
    }
    return true;
}

void Loop::find_bound() {
    for (auto iv : get_ind_vars()) {
        if (iv->type == InductionVar::IndVarType::basic) {
            if (iv->initial_val && iv->step_val && iv->final_val) {
                bound_IV = iv;
                return;
            }
        }
    }
}

void Loop::find_break() {
    for (auto bb : get_blocks()) {
        if (latch_blocks.find(bb) != latch_blocks.end())
            continue;
        for (auto inst : bb->get_instructions()) {
            if (inst->is_br()) {
                auto br = std::static_pointer_cast<BranchInst>(inst);
                if (!br->is_cond_br() && !contains(br->get_target().get())) {
                    has_break_ = true;
                    return;
                } else if (br->is_cond_br()) {
                    if (!contains(br->get_true_succ().get()) || !contains(br->get_false_succ().get())) {
                        has_break_ = true;
                        return;
                    }
                }
            }
        }
    }
    has_break_ = false;
}
