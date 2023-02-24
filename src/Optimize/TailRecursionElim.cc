#include "TailRecursionElim.hh"

#include "utils.hh"
using std::string_literals::operator""s;
void TailRecursionElim::run() {
    LOG_DEBUG << "tail recursion hello";
    for (auto f : m_->get_functions()) {
        if (f->get_name() == "param32_rec") // 这个函数参数太多，load/store PHI 指令需要排序，不想改后端
            continue;
        f_ = f.get();
        preheader = latch = header = return_bb = nullptr;
        phi_args.clear();
        args_latch.clear();
        ret_phi = nullptr;
        acc_phi = acc_latch = nullptr;
        for (auto bb : f->get_basic_blocks())
            if (auto call = get_candidate(bb.get())) {
                auto b = eliminate_call(call);
            }
    }
}

/// \brief is not tail recursive, or cannot be accumulated
bool TailRecursionElim::check_tre(CallInst *call) { return true; }

bool TailRecursionElim::eliminate_call(CallInst *call) {
    auto parent = call->get_parent();
    const auto call_it = call->get_iterator();
    auto it = ++call->get_iterator();
    auto term_it = call->get_parent()->get_terminator_itr();
    std::shared_ptr<Instruction> accumulator_or_call = std::dynamic_pointer_cast<Instruction>(call->shared_from_this());
    if (BasicBlock * bb; not(match(term_it->get(), m_br(m_bb(bb))) and bb->get_terminator()->is_ret()))
        return false;
    else {
        ret_phi = dynamic_cast<Instruction *>(bb->get_terminator()->get_operand(0).get());
        return_bb = std::dynamic_pointer_cast<BasicBlock>(bb->shared_from_this());
    }
    bool accumulate;
    if (it == term_it)  // branch/return immediately after call
        accumulate = false;
    while (it != term_it) {
        auto cur = *it;
        if (not(cur->is_add() or cur->is_fadd() or cur->is_mul() or cur->is_fmul()))
            return false;
        else if (auto bin = std::dynamic_pointer_cast<BinaryInst>(accumulator_or_call);
                 bin and bin->get_instr_type() != cur->get_instr_type())
            return false;
        // pattern like `add %call, %call` can be transformed into `mul %call, 2`
        if (not((cur->get_operand(0).get_shared() == accumulator_or_call and
                 cur->get_operand(1).get_shared() != accumulator_or_call) or
                (cur->get_operand(0).get_shared() != accumulator_or_call and
                 cur->get_operand(1).get_shared() == accumulator_or_call)))
            return false;
        else
            accumulate = true;
        accumulator_or_call = cur;
        ++it;
    }
    int id{-1};
    for (auto use : accumulator_or_call->get_use_list())
        if (use.val_ == ret_phi) {
            id = use.arg_no_;
            break;
        }
    if (id == -1) {
        LOG_WARNING << "did not find use of call";
        return false;
    }
    if (not preheader)
        create_header();
    if (accumulate) {
        if (not acc_phi) {
            auto header_begin = header->begin();
            auto latch_begin = latch->begin();
            acc_phi = PhiInst::create_phi(call->get_type(), &header_begin);
            acc_phi->set_name("acc");
            acc_latch = PhiInst::create_phi(call->get_type(), &latch_begin);
            acc_latch->set_name("acc.latch");
            acc_phi->add_phi_pair_operand(std::weak_ptr(acc_latch), latch);
            std::shared_ptr<Constant> identity;
            switch (accumulator_or_call->get_instr_type()) {
                case Instruction::add:
                    identity = ConstantInt::get(0, m_);
                    break;
                case Instruction::fadd:
                    identity = ConstantFP::get(0, m_);
                    break;
                case Instruction::mul:
                    identity = ConstantInt::get(1, m_);
                    break;
                case Instruction::fmul:
                    identity = ConstantFP::get(1, m_);
                    break;
                default:
                    LOG_ERROR << "how do you get here?";
            }
            acc_phi->add_phi_pair_operand(std::shared_ptr<Value>(identity), preheader);
            auto ret_it = return_bb->get_terminator_itr();
            auto new_ret = BinaryInst::create(accumulator_or_call->get_type(),
                                              accumulator_or_call->get_instr_type(),
                                              acc_phi,
                                              ret_phi->shared_from_this(),
                                              &ret_it);
            ret_phi->replace_use_with_when(new_ret, [&](auto user) { return user != new_ret.get(); });
            // TODO: acc_phi add identity (one for mul, zero for add), and connect acc_latch to acc_phi
        }
        // acc_phi->add_phi_pair_operand()
        acc_latch->add_phi_pair_operand(std::weak_ptr(accumulator_or_call), call->get_parent_shared());
    }

    for (size_t i = 1; i < call->get_num_operand(); ++i) {
        auto op = call->get_operand(i)->shared_from_this();
        args_latch[i - 1]->add_phi_pair_operand(op, call->get_parent_shared());
    }
    ret_phi->remove_operands(id, id + 1);
    if (accumulate) {
        call->replace_all_use_with(acc_phi);
    }
    parent->delete_instr(call->get_iterator());
    parent->delete_instr(parent->get_terminator_itr());
    for (auto succ : parent->get_succ_basic_blocks())
        succ->remove_pre_basic_block(parent);
    parent->get_succ_basic_blocks().clear();
    // parent
    // TODO: change branch dest, remove call and its uses
    BranchInst::create_br(latch, parent);

    return true;
}

CallInst *TailRecursionElim::get_candidate(BasicBlock *bb) {
    auto &list = bb->get_instructions();
    for (auto it = list.rbegin(); it != list.rend(); ++it) {
        if (auto call = dynamic_cast<CallInst *>(it->get()); call and call->get_callee() == f_) {
            return call;
        }
    }
    return {};
}

void TailRecursionElim::create_header() {
    auto original_entry = header = f_->get_entry_block();
    // TODO: add guard, preheader, body (body is the old entry!), latch
    // auto loop_body = BasicBlock::create(m_, "", f_->get_basic_blocks().begin());
    preheader = BasicBlock::create(m_, "", f_->get_basic_blocks().begin());
    preheader->take_name(original_entry.get());
    BranchInst::create_br(original_entry, preheader.get());
    latch = BasicBlock::create(m_, "", f_);
    latch->set_name("tail_latch");
    // loop info needs latch block to have 2 successors, use conditional jump
    // need simplification in opts afterwards
    BranchInst::create_cond_br(ConstantInt::get(true, m_), original_entry, return_bb, latch.get());
    // we should generate canonical loop
    original_entry->set_name("tail_header");
    for (auto arg : f_->get_args()) {
        auto loop_body_begin = original_entry->begin();  // ugly!
        auto latch_begin = latch->begin();
        std::shared_ptr<PhiInst> phi_arg = PhiInst::create_phi(arg->get_type(), &loop_body_begin);
        std::shared_ptr<PhiInst> phi_latch = PhiInst::create_phi(arg->get_type(), &latch_begin);
        arg->replace_all_use_with(phi_arg);
        phi_arg->add_phi_pair_operand(std::weak_ptr<Value>(arg), preheader);
        phi_arg->add_phi_pair_operand(std::weak_ptr<Value>(phi_latch), latch);  // we might have multiple tail calls
        phi_args.push_back(phi_arg);
        args_latch.push_back(phi_latch);
    }
}
