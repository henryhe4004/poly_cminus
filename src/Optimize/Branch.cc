#include "Branch.hh"

#include "utils.hh"

#include <queue>
void Branch::run() {
    jump_table();
    replace_phi_with_select();
    // replace_and_or();
}

void Branch::replace_phi_with_select() {
    for (auto f : m_->get_functions()) {
        bool changed;
        do {
            changed = false;
            changed |= try_optimize_phi(f.get());
        } while (changed);
    }
}

void Branch::replace_and_or() {
    for (auto f : m_->get_functions()) {
        bool changed;
        do {
            changed = false;
            changed |= try_optimize_and_or(f.get());
        } while (changed);
    }
}

bool Branch::try_optimize_and_or(Function *func) {
    for (auto bb : func->get_basic_blocks()) {
        auto conditions = identity_condition_blocks(bb.get(), false);
        if (conditions.size() != 2)
            continue;
        if (analyze(conditions) == pattern::land) {
            auto last = std::get<0>(conditions.back());
            auto last_cond = dynamic_cast<Instruction *>(last);
            auto first = std::get<0>(conditions.front());
            auto first_block = dynamic_cast<Instruction *>(first)->get_parent();
            auto true_block = std::dynamic_pointer_cast<BasicBlock>(std::get<1>(conditions.back())->shared_from_this());
            auto false_block =
                std::dynamic_pointer_cast<BasicBlock>(std::get<2>(conditions.back())->shared_from_this());
            last_cond->get_parent()->get_instructions().erase(last_cond->get_iterator());
            first_block->insert_before_terminator(
                std::dynamic_pointer_cast<Instruction>(last_cond->shared_from_this()));
            last_cond->set_parent(first_block);
            auto and_inst = BinaryInst::create_and(first->shared_from_this(), last->shared_from_this(), nullptr);
            first_block->insert_before_terminator(and_inst);
            and_inst->set_parent(first_block);
            first_block->delete_instr(first_block->get_terminator_itr());
            for (auto succ : first_block->get_succ_basic_blocks())
                succ->remove_pre_basic_block(first_block);
            first_block->get_succ_basic_blocks().clear();
            BranchInst::create_cond_br(and_inst, true_block, false_block, first_block);
        }
    }
    return false;
}

bool Branch::try_optimize_phi(Function *func) {
    BasicBlock* input_true,*input_false;
    auto select_replaced = [&input_true, &input_false](BasicBlock *bb) -> BasicBlock * {
        // bb has a conditional branch inst
        auto cond_br = bb->get_terminator();
        auto true_bb = dynamic_cast<BasicBlock *>(cond_br->get_operand(1).get());
        auto false_bb =dynamic_cast<BasicBlock *>(cond_br->get_operand(2).get());
        if (true_bb->get_succ_basic_blocks().size() != 1 or false_bb->get_succ_basic_blocks().size() != 1)
            return nullptr;
        auto true_bb_succ = true_bb->get_succ_basic_blocks().front();
        auto false_bb_succ = false_bb->get_succ_basic_blocks().front();
        BasicBlock *target{};
        std::vector<BasicBlock *> buf{};
        const int max_instruction_num = 2;
        if (true_bb_succ == false_bb and true_bb->get_instructions().size() <= max_instruction_num) {
            target = false_bb;
            input_true = true_bb;
            input_false = bb;
            buf.push_back(true_bb);
        } else if (false_bb_succ == true_bb and false_bb->get_instructions().size() <= max_instruction_num) {
            target = true_bb;
            input_true = bb;
            input_false = false_bb;
            buf.push_back(false_bb);
        } else if (true_bb_succ == false_bb_succ and true_bb->get_instructions().size() <= max_instruction_num and
                   false_bb->get_instructions().size() <= max_instruction_num) {
            target = false_bb_succ;
            input_true = true_bb;
            input_false = false_bb;
            buf.push_back(true_bb);
            buf.push_back(false_bb);
        }
        if (target) {
            auto [phi_begin, phi_end] = target->get_phis();
            if (phi_begin == phi_end)
                return nullptr;
            // move calculations (the one before branch) to `bb`
            for (auto bb_to_del : buf) {
                for (auto it = bb_to_del->begin(); it != bb_to_del->get_terminator_itr();) {
                    bb->insert_before_terminator(*it);
                    (*it)->set_parent(bb);
                    // we can't use delete_instr, which removes use-def information
                    it = bb_to_del->get_instructions().erase(it);
                }
            }
            return target;
        }
        return nullptr;
    };

    for (auto bb : func->get_basic_blocks()) {
        if (not(isa<BranchInst>(bb->get_terminator()) and
                dynamic_cast<BranchInst *>(bb->get_terminator().get())->is_cond_br()))
            continue;
        auto cond_inst = dynamic_cast<Instruction *>(bb->get_terminator()->get_operand(0).get());
        if (auto targetbb = select_replaced(bb.get())) {
            if (targetbb->get_pre_basic_blocks().size() > 2)
                continue;
            auto term_it = bb->get_terminator_itr();
            auto [phi_begin, phi_end] = targetbb->get_phis();
            for (auto it = phi_begin; it != phi_end;) {
                auto PHI = dynamic_cast<PhiInst*>(it->get());
                // choose lhs/rhs!
                auto sel = SelectInst::create_select(cond_inst->shared_from_this(),
                                                     PHI->input_of(input_true)->shared_from_this(),
                                                     PHI->input_of(input_false)->shared_from_this(),
                                                     &term_it);
                PHI->replace_all_use_with(sel);
                it = targetbb->delete_instr(it);
            }
            bb->delete_instr(term_it);
            for (auto succ : bb->get_succ_basic_blocks())
                succ->remove_pre_basic_block(bb.get());
            bb->get_succ_basic_blocks().clear();
            BranchInst::create_br(std::dynamic_pointer_cast<BasicBlock>(targetbb->shared_from_this()), bb.get());
            return true;
        }
    }
    return false;
}

void Branch::jump_table() {
    for (auto f : m_->get_functions()) {
        bool changed;
        do {
            changed = false;
            changed |= try_optimize_jump_table(f.get());
        } while (changed);
    }
}

Branch::pattern Branch::analyze(const std::list<std::tuple<Value *, BasicBlock *, BasicBlock *>> &conditions) {
    assert(not conditions.empty() and "condition blocks must be non-empty");
    auto first_false_block = std::get<2>(conditions.front());
    // std::get<1>(conditions.front());
    auto next_true_block = dynamic_cast<Instruction *>(std::get<0>(conditions.front()))->get_parent();
    bool land = true;
    for (auto &[cond, t, f] : conditions) {
        if (f != first_false_block) {
            land = false;
            break;
        }
        if (auto cond_inst = dynamic_cast<Instruction *>(cond); next_true_block != cond_inst->get_parent()) {
            land = false;
            break;
        }
        next_true_block = t;
    }
    if (land)
        return pattern::land;
    bool one_each = true;
    auto cond_block = dynamic_cast<Instruction *>(std::get<0>(conditions.front()))->get_parent();
    for (auto &[cond, t, f] : conditions) {
        if (dynamic_cast<Instruction *>(cond)->get_parent() != cond_block) {
            one_each = false;
            break;
        }
        cond_block = f;
    }
    if (one_each)
        return pattern::one_each;
    return pattern::unknown;
}

bool Branch::try_optimize_jump_table(Function *func) {
    for (auto bb : func->get_basic_blocks()) {
        auto conditions = identity_condition_blocks(bb.get(), true);
        if (conditions.size() <= 4)
            continue;
        auto [vv, true_bb, false_bb] = conditions.front();
        auto cond_inst = dynamic_cast<CmpInst *>(vv);
        auto lhs = cond_inst->get_operand(0).get();
        auto cond_op = cond_inst->get_cmp_op();
        auto common_lhs = std::all_of(conditions.begin(), conditions.end(), [&](auto block) {
            auto [v, t, f] = block;
            auto cond_inst = dynamic_cast<CmpInst *>(v);
            return cond_inst and cond_inst->get_operand(0).get() == lhs and
                   isa<ConstantInt>(cond_inst->get_operand(1).get()) and cond_inst->get_cmp_op() == cond_op;
        });

        if (common_lhs) {
            pattern p = analyze(conditions);
            // 接下来判断各个condition跳转到哪里，brainfuck这个示例有两种情况
            // 1. Op是NE时都跳到同一块
            if (p == pattern::land and cond_op == (CmpOp)CmpOp::NE) {
                // 全是and的话最后才有结果
                auto last_condition = dynamic_cast<Instruction *>(std::get<0>(conditions.back()));
                auto last_block = last_condition->get_parent();
                auto success =
                    std::dynamic_pointer_cast<BasicBlock>(std::get<1>(conditions.back())->shared_from_this());
                auto failure_block =
                    std::dynamic_pointer_cast<BasicBlock>(std::get<2>(conditions.back())->shared_from_this());
                auto first_condition = dynamic_cast<Instruction *>(std::get<0>(conditions.front()));
                auto insert_block = first_condition->get_parent();
                auto switch_inst = SwitchInst::create_switch(lhs->shared_from_this(), success);
                for (auto [v, t, f] : conditions) {
                    auto cond_inst = dynamic_cast<CmpInst *>(v);
                    switch_inst->add_switch_pair(cond_inst->get_operand(1)->shared_from_this(), failure_block);
                    auto cond_br = cond_inst->get_parent()->get_terminator();
                    // 需要改写condition中所有的conditional jump，以免下一次迭代再次发现同样的bbs(或其子集)
                    // cond_br->remove_operands(0, 1);
                    cond_inst->remove_use(cond_br.get());
                    cond_br->set_operand(0, std::static_pointer_cast<Value>(ConstantInt::get(true, m_)));
                }
                insert_block->delete_instr(insert_block->get_terminator_itr());
                insert_block->add_instruction(switch_inst);
                switch_inst->set_parent(insert_block);
                for (auto succ : insert_block->get_succ_basic_blocks()) {
                    succ->remove_pre_basic_block(insert_block);
                }
                insert_block->get_succ_basic_blocks().clear();
                insert_block->add_succ_basic_block(success.get());
                success->add_pre_basic_block(insert_block);
                insert_block->add_succ_basic_block(failure_block.get());
                failure_block->add_pre_basic_block(insert_block);
                LOG_DEBUG << switch_inst->print();
                return true;
            }
            // 2. EQ时跳到各自不同的块
            else if (p == pattern::one_each and cond_op == (CmpOp)CmpOp::EQ) {
                LOG_DEBUG << "todo";
                auto default_block =
                    std::dynamic_pointer_cast<BasicBlock>(std::get<2>(conditions.back())->shared_from_this());
                auto first_cond = dynamic_cast<Instruction *>(std::get<0>(conditions.front()));
                auto insert_block = first_cond->get_parent();
                auto switch_inst = SwitchInst::create_switch(lhs->shared_from_this(), default_block);
                for (auto succ : insert_block->get_succ_basic_blocks()) {
                    succ->remove_pre_basic_block(insert_block);
                }
                insert_block->get_succ_basic_blocks().clear();
                insert_block->add_succ_basic_block(default_block.get());
                default_block->add_pre_basic_block(insert_block);
                for (auto [v, t, f] : conditions) {
                    auto op = dynamic_cast<Instruction *>(v)->get_operand(1)->shared_from_this();
                    auto t_sh = std::dynamic_pointer_cast<BasicBlock>(t->shared_from_this());
                    switch_inst->add_switch_pair(op, t_sh);
                    insert_block->add_succ_basic_block(t);
                    t->add_pre_basic_block(insert_block);

                    auto cond_inst = dynamic_cast<Instruction *>(v);
                    auto cond_br = cond_inst->get_parent()->get_terminator();
                    cond_inst->remove_use(cond_br.get());
                    cond_br->set_operand(0, std::static_pointer_cast<Value>(ConstantInt::get(true, m_)));
                }
                insert_block->delete_instr(insert_block->get_terminator_itr());
                insert_block->add_instruction(switch_inst);
                switch_inst->set_parent(insert_block);
            }
        }
    }
    return false;
}

std::list<std::tuple<Value *, BasicBlock *, BasicBlock *>> Branch::identity_condition_blocks(BasicBlock *bb,
                                                                                             bool lhs_same) {
    auto local_pure_jump = [](BasicBlock *bb) {
        return bb->get_terminator()->is_br() and bb->get_terminator()->get_num_operand() == 3 and
               not isa<ConstantInt>(bb->get_terminator()->get_operand(0).get());
    };
    if (not local_pure_jump(bb))
        return {};
    std::list<std::tuple<Value *, BasicBlock *, BasicBlock *>> conditions;
    std::queue<BasicBlock *> bbs{};
    bbs.push(bb);
    Value *lhs{};
    while (not bbs.empty()) {
        auto bb = bbs.front();
        bbs.pop();
        auto term = bb->get_terminator();
        if (term->is_br() and term->get_num_operand() == 3) {
            auto br = dynamic_cast<BranchInst *>(term.get());
            auto cond_inst = dynamic_cast<Instruction *>(term->get_operand(0).get());
            if (lhs_same and
                (lhs == nullptr or
                 lhs == cond_inst->get_operand(0).get())) {  // 这里对lhs有限制，不判断一下的话跳转表有时候找不到
                lhs = cond_inst->get_operand(0).get();
                conditions.push_back(
                    {term->get_operand(0).get(), br->get_true_succ().get(), br->get_false_succ().get()});
            } else if (not lhs_same) {
                conditions.push_back(
                    {term->get_operand(0).get(), br->get_true_succ().get(), br->get_false_succ().get()});
                if (conditions.size() >= 2)  // one by one
                    break;
            }
        }
        for (auto succ : bb->get_succ_basic_blocks()) {
            if (succ->get_instructions().size() == 2 and local_pure_jump(succ))
                bbs.push(succ);
        }
    }
    LOG_DEBUG << "Conditions: ";
    for (auto [val, t, f] : conditions)
        LOG_DEBUG << val->print() << " ? " << t->get_name() << " : " << f->get_name();

    return conditions;
}
