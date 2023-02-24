#include "IVReduction.hh"

#include "DeadCode.hh"
#include "utils.hh"

void IVReduction::run() {
    dependent_IV_red();
    IV_final_val();
    common_iv_elim();
}

class ivhash {
  public:
    std::size_t operator()(std::shared_ptr<Loop::InductionVar> now) const {
        return reinterpret_cast<uintptr_t>(now->initial_val.get());
    };
};

class ivequal {
  public:
    bool operator()(const std::shared_ptr<Loop::InductionVar> &lhs,
                    const std::shared_ptr<Loop::InductionVar> &rhs) const {
        return lhs->initial_val == rhs->initial_val and lhs->step_val == rhs->step_val;
    };
};

using ivset = std::unordered_set<std::shared_ptr<Loop::InductionVar>, ivhash, ivequal>;

void IVReduction::common_iv_elim() {
    LoopInfo loop_info(m_);
    loop_info.run();
    for (auto loop : loop_info.get_loops()) {
        ivset s{};
        for (auto iv : loop.get_ind_vars()) {
            if (s.count(iv)) {
                auto common = *s.find(iv);
                iv->inst->replace_all_use_with(common->inst);
            } else
                s.insert(iv);
        }
    }
}
void IVReduction::dependent_IV_red() {
    // debug
    // std::cout << m_->print();
    LOG_INFO << "iv reduction pass";
    LoopInfo loop_info(m_);
    loop_info.run();
    for (auto loop : loop_info.get_loops()) {
        loop.find_dependent_IV();
        for (auto iv : loop.get_ind_vars()) {
            if (iv->type == ivtype::dependent) {
                auto const_c1 = utils::get_const_int_val(iv->c1.get());
                auto const_c2 = utils::get_const_int_val(iv->c2.get());
                auto basis = iv->basis;
                if (const_c1.has_value() && const_c1.value() == 1)
                    continue;

                /// 对于下面这样的循环：
                /// for (int basic = init_b; basic < final_b; basic += step_b) {
                ///     dependent = basic * c1 + c2;
                /// }
                /// initial value of dependent: init_b * c1 + c2 - step
                /// step size of dependent: step_b * c1
                /// final value of dependent: final_b * c1 + c2

                auto step = utils::mul_i32(iv->c1, iv->basis->step_val, m_);
                auto init_mul_part = utils::mul_i32(iv->c1, iv->basis->initial_val, m_);
                std::shared_ptr<Value> init_full;
                if (iv->basis->bin_op == Instruction::add)
                    init_full = utils::sub_i32(init_mul_part, step, m_);
                else
                    init_full = utils::add_i32(init_mul_part, step, m_);
                std::shared_ptr<Value> init;
                if (const_c2.has_value() && const_c2.value() == 0)
                    init = init_full;
                else
                    init = utils::add_i32(init_full, iv->c2, m_);
                loop.insert_into_preheader(step);
                loop.insert_into_preheader(init_mul_part);
                loop.insert_into_preheader(init_full);
                if (init != init_full)
                    loop.insert_into_preheader(init);
                auto new_iv_phi = PhiInst::create_phi(Type::get_int32_type(m_));
                new_iv_phi->add_phi_pair_operand(init, loop.get_preheader());
                auto new_step_inst = std::static_pointer_cast<Instruction>(iv->basis->bin_op == Instruction::add
                                                                               ? utils::add_i32(new_iv_phi, step, m_)
                                                                               : utils::sub_i32(new_iv_phi, step, m_));
                new_iv_phi->add_phi_pair_operand(std::weak_ptr<Value>(new_step_inst),
                                                 std::static_pointer_cast<BasicBlock>(loop.get_end()));
                loop.insert_phi_at_begin(new_iv_phi);
                // 新指令插入循环开始处
                loop.insert_at_begin(new_step_inst);

                iv->inst->replace_all_use_with(new_step_inst);
                // iv->inst->remove_use_of_ops();
                iv->inst->get_parent()->delete_instr(iv->inst);

                // 试图求出final值
                // auto exit_ter = loop.get_exit()->get_terminator_itr();
                // std::shared_ptr<Value> iv_final;
                // if (basis->final_val) {
                //     if (basis->bin_op == Instruction::add) {
                //         /// while (i < final) {
                //         ///     j = i * b + c
                //         ///     i = i + step
                //         /// }
                //         /// after loop: j = (final - step) * b + c
                //         if (basis->predicate == CmpOp::LE) {
                //             auto f_sub_s = utils::sub_i32(basis->final_val, basis->step_val, m_, &exit_ter);
                //             auto fs_mul_b = utils::mul_i32(f_sub_s, iv->c1, m_, &exit_ter);
                //             auto fsmb_add_c = utils::add_i32(fs_mul_b, iv->c2, m_, &exit_ter);
                //             iv_final = fsmb_add_c;
                //         } else if (basis->predicate == CmpOp::LT || basis->predicate == CmpOp::NE) {

                //         }
                //     }
                // }
                LOG_DEBUG << "weaken dependent iv: " << iv->inst->get_name() << " in func "
                          << loop.get_base()->get_parent()->get_name() << " bb " << iv->inst->get_parent()->get_name();
            }
        }
    }
    // debug
    // std::cout << m_->print();
}

void IVReduction::IV_final_val() {
    DeadCode{m_}.run();
    LoopInfo loop_info(m_);
    loop_info.run();
    for (auto loop : loop_info.get_loops()) {
        auto bound = loop.get_bound_IV();
        if (!bound)
            continue;
        auto bound_step = bound->step_val;
        int const_bound_step;
        if (auto c_step = bound->get_const_step_val(); c_step.has_value())
            const_bound_step = c_step.value();
        else
            continue;
        // if (const_bound_step != 1)
        //     continue;
        auto exit_ter = loop.get_exit()->get_terminator_itr();
        std::shared_ptr<Value> exe_time;
        if (bound->bin_op == Instruction::add && const_bound_step > 0) {
            auto interval = utils::sub_i32(bound->final_val, bound->initial_val, m_, &exit_ter);
            if (bound->predicate == (CmpOp)CmpOp::LT) {
                interval = utils::sub_i32(interval, ConstantInt::get(1, m_), m_, &exit_ter);
            } else if (bound->predicate != (CmpOp)CmpOp::LE)
                continue;
            exe_time = utils::sdiv_i32(interval, bound->step_val, m_, &exit_ter);
            exe_time = utils::add_i32(exe_time, ConstantInt::get(1, m_), m_, &exit_ter);
        }
        if (!exe_time)
            continue;
        for (auto iv : loop.get_ind_vars()) {
            auto final_val =
                utils::add_i32(iv->initial_val, utils::mul_i32(iv->step_val, exe_time, m_, &exit_ter), m_, &exit_ter);

            // 用计算出来的终止值替换循环之后的iv->step_inst
            auto use_list_copy = iv->step_inst->get_use_list();
            for (auto use : use_list_copy) {
                auto use_inst = dynamic_cast<Instruction *>(use.val_);
                if (use_inst && !loop.contains(use_inst)) {
                    use_inst->set_operand(use.arg_no_, final_val);
                    iv->step_inst->remove_use(use.val_);
                }
            }
        }
    }
}