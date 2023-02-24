#include "LoopUnrolling.hh"

#include <map>
#include <vector>

using std::map;

Module *module;

void replace_calculate_inst(vector<Instruction *> &calculate_inst, vector<Instruction *> &new_calculate_inst) {
    for (int i = 0; i < calculate_inst.size(); i++)
        calculate_inst[i]->replace_all_use_with(new_calculate_inst[i]->shared_from_this());
}

void unroll_base_block(Loop loop, int unroll_times, map<Value *, Value *> &phi_to_compare_block_value) {
    auto base_block = loop.get_base();
    LOG_DEBUG << "unroll base block: " << base_block->get_name() << " " << unroll_times;

    vector<PhiInst *> phi_inst;
    vector<Instruction *> calculate_inst;
    for (auto inst : base_block->get_instructions()) {
        if (inst->is_phi())
            phi_inst.push_back(static_cast<PhiInst *>(inst.get()));
        else if (not inst->is_br())
            calculate_inst.push_back(inst.get());
    }

    map<Instruction *, int> calculate_inst_to_index;
    int index = 0;
    for (auto inst : calculate_inst) {
        calculate_inst_to_index[inst] = index;
        index++;
    }

    // step1: 删除 br
    base_block->delete_instr(base_block->get_instructions().back());

    auto compare_block = loop.get_latch();
    map<Value *, Value *> phi_to_value, new_phi_to_value;
    vector<Instruction *> new_calculate_inst;

    // step2: 添加指令
    for (int i = 0; i < unroll_times; i++) {
        phi_to_value = new_phi_to_value;
        new_phi_to_value.clear();

        new_calculate_inst = utils::clone_instructions(calculate_inst, base_block.get(), phi_to_value);
        assert(new_calculate_inst.size() == calculate_inst.size());

        // 维护 phi_to_value
        for (auto [phi, compare_value] : phi_to_compare_block_value) {
            auto inst = dynamic_cast<Instruction *>(compare_value);
            if (inst != nullptr)
                new_phi_to_value[phi] = new_calculate_inst[calculate_inst_to_index[inst]];
        }
    }

    // step3: 替换 calculate_inst 为 new_calculate_inst
    replace_calculate_inst(calculate_inst, new_calculate_inst);

    // step4: 删除 calculate_inst
    for (auto inst : calculate_inst)
        base_block->delete_instr(std::static_pointer_cast<Instruction>(inst->shared_from_this()));

    // step5: 添加 br
    BranchInst::create(compare_block, base_block.get());
}

void unroll_pre_block(Loop loop,
                      int unroll_times,
                      map<Value *, Value *> &phi_to_pre_value,
                      map<Value *, int> &phi_to_pre_index,
                      map<Value *, Value *> &phi_to_compare_block_value,
                      bool skip_loop) {
    auto pre_block = loop.get_preheader();
    auto base_block = loop.get_base();
    auto compare_block = loop.get_latch();
    auto exit_block = loop.get_exit();

    LOG_DEBUG << "unroll pre block: " << pre_block->get_name() << " " << unroll_times;

    vector<PhiInst *> phi_inst;
    vector<Instruction *> calculate_inst;
    for (auto inst : base_block->get_instructions()) {
        if (inst->is_phi())
            phi_inst.push_back(static_cast<PhiInst *>(inst.get()));
        else if (not inst->is_br())
            calculate_inst.push_back(inst.get());
    }

    map<Instruction *, int> calculate_inst_to_index;
    int index = 0;
    for (auto inst : calculate_inst) {
        calculate_inst_to_index[inst] = index;
        index++;
    }

    // step1: 删除 br
    pre_block->delete_instr(pre_block->get_instructions().back());

    map<Value *, Value *> phi_to_value, new_phi_to_value;
    new_phi_to_value = phi_to_pre_value;

    // step2: 添加指令
    vector<Instruction *> new_calculate_inst;
    for (int i = 0; i < unroll_times; i++) {
        phi_to_value = new_phi_to_value;

        new_calculate_inst = utils::clone_instructions(calculate_inst, pre_block.get(), phi_to_value);
        assert(new_calculate_inst.size() == calculate_inst.size());

        // 维护 phi_to_value
        new_phi_to_value.clear();
        for (auto [phi, compare_value] : phi_to_compare_block_value) {
            auto inst = dynamic_cast<Instruction *>(compare_value);
            assert(inst != nullptr);
            new_phi_to_value[phi] = new_calculate_inst[calculate_inst_to_index[inst]];
        }
    }

    if (skip_loop) {
        // 替换为 new_calculate_inst
        replace_calculate_inst(calculate_inst, new_calculate_inst);

        // 更新 exit_block 中的 phi 的 compare_block 为 pre_block
        for (auto inst : exit_block->get_instructions()) {
            auto phi = dynamic_cast<PhiInst *>(inst.get());
            if (phi == nullptr)
                continue;
            int pair_num = phi->get_num_operand() / 2;
            for (int i = 0; i < pair_num; i++) {
                auto from_bb = phi->get_operand(i * 2 + 1).get();
                if (from_bb == compare_block.get())
                    phi->set_operand(i * 2 + 1, pre_block->shared_from_this());
            }
        }

        // 删除 base_block 和 compare_block
        auto func = base_block->get_parent();
        func->remove_unreachable_basic_block(base_block);
        func->remove_unreachable_basic_block(compare_block);

        BranchInst::create(exit_block, pre_block.get());
        pre_block->add_succ_basic_block(exit_block.get());
        exit_block->add_pre_basic_block(pre_block.get());
        pre_block->remove_succ_basic_block(base_block.get());
        base_block->remove_pre_basic_block(pre_block.get());
    } else {
        // step3: 替换 init_phi_to_value 中的 val 为 new_phi_to_value
        for (auto [phi, value] : phi_to_pre_value) {
            auto phi_inst = static_cast<PhiInst *>(phi);

            // remove use
            auto operand = phi_inst->get_operand(phi_to_pre_index[phi]);
            operand->remove_use(phi_inst);

            phi_inst->set_operand(phi_to_pre_index[phi], new_phi_to_value[phi]->shared_from_this());
        }

        // step4: 添加 br
        BranchInst::create(base_block, pre_block.get());
    }
}

void clone_and_new_loop(Loop loop, int unroll_base_times) {
    auto pre_block = loop.get_preheader();
    auto base_block = loop.get_base();
    auto compare_block = loop.get_latch();
    auto exit_block = loop.get_exit();

    assert(pre_block->get_pre_basic_blocks().size() == 1);
    assert(exit_block->get_succ_basic_blocks().size() == 1);

    auto before_block =
        std::dynamic_pointer_cast<BasicBlock>(pre_block->get_pre_basic_blocks().front()->shared_from_this());
    auto after_block =
        std::dynamic_pointer_cast<BasicBlock>(exit_block->get_succ_basic_blocks().front()->shared_from_this());

    LOG_DEBUG << "pre_block: " << pre_block->get_name();
    LOG_DEBUG << "base_block: " << base_block->get_name();
    LOG_DEBUG << "compare_block: " << compare_block->get_name();
    LOG_DEBUG << "after_block: " << after_block->get_name();
    LOG_DEBUG << "exit_block: " << exit_block->get_name();

    LOG_INFO << "clone and new loop: " << base_block->get_name() << " " << unroll_base_times;

    auto ind_var = loop.get_ind_vars().begin()->get();
    auto initial_val = ind_var->initial_val;
    auto final_val = ind_var->final_val;
    auto step_val = ind_var->step_val;
    auto step_const = dynamic_cast<ConstantInt *>(step_val.get())->get_value();
    auto cmp_op = ind_var->predicate;

    int unroll_size;
    if (cmp_op == (CmpOp)CmpOp::LT or cmp_op == (CmpOp)CmpOp::LE)
        unroll_size = unroll_base_times * step_const;
    else if (cmp_op == (CmpOp)CmpOp::GT or cmp_op == (CmpOp)CmpOp::GE)
        unroll_size = -unroll_base_times * step_const;
    else
        return;

    LOG_DEBUG << "unroll_margin: " << unroll_size;

    // TODO: br 前不一定是 compare_inst，可能需要更改
    if (before_block->get_instructions().size() <= 1)
        return;
    auto before_compare_inst_pos = before_block->get_instructions().end().operator--().operator--();
    auto before_compare_inst = dynamic_cast<CmpInst *>(before_compare_inst_pos->get());
    if (not before_compare_inst)
        return;

    if (final_val == nullptr)
        return;
    if (not compare_block->get_instructions().front()->is_cmp())
        return;

    LOG_DEBUG << "clone and unroll loop: " << base_block->get_name() << " " << compare_block->get_name();

    map<Value *, Value *> to_new_operand;

    auto new_before_block = utils::init_create_block(before_block.get(), "_new_before");
    auto new_pre_block = utils::init_create_block(compare_block.get(), "_new_pre");
    auto new_base_block = utils::init_create_block(base_block.get(), "_new_base");
    auto new_compare_block = utils::init_create_block(compare_block.get(), "_new_compare");
    auto new_exit_block = utils::init_create_block(exit_block.get(), "_new_exit");
    auto new_after_block = utils::init_create_block(after_block.get(), "_new_after");

    // 原始：
    // before_block                 l 与 r 比较         --> pre_block       --> after_block
    // pre_block
    // base_block                   每次加 1
    // compare_block                i 与 r 比较         --> base_block      --> exit_block
    // exit_block
    // after_block

    // 新：
    // before_block                 l 与 r - 8 比较     --> pre_block       --> exit_block
    // pre_block
    // base_block                   每次加 8
    // compare_block                i 与 r - 8 比较     --> base_block      --> after_block
    // exit_block
    // after_block

    // new_before_block             i 与 r 比较         --> new_pre_block   --> after_block
    // new_pre_block
    // new_base_block               每次加 1
    // new_compare_block            i 与 r 比较         --> new_base_block  --> exit_block
    // new_exit_block
    // new_after_block

    // 1. 复制块
    to_new_operand[before_block.get()] = new_before_block.get();
    to_new_operand[pre_block.get()] = new_pre_block.get();
    to_new_operand[base_block.get()] = new_base_block.get();
    to_new_operand[compare_block.get()] = new_compare_block.get();
    to_new_operand[exit_block.get()] = new_exit_block.get();
    to_new_operand[after_block.get()] = new_after_block.get();

    utils::clone_blocks({before_block.get()}, {new_before_block}, to_new_operand);
    utils::clone_blocks({pre_block.get()}, {new_pre_block}, to_new_operand);
    utils::clone_blocks({base_block.get()}, {new_base_block}, to_new_operand);
    utils::clone_blocks({compare_block.get()}, {new_compare_block}, to_new_operand);
    utils::clone_blocks({exit_block.get()}, {new_exit_block}, to_new_operand);
    utils::clone_blocks({after_block.get()}, {new_after_block}, to_new_operand);

    utils::replace_phi(
        {new_before_block, new_pre_block, new_base_block, new_compare_block, new_exit_block, new_after_block},
        to_new_operand);

    // 2. 处理跳转关系
    // after_block 跳转到 new_before_block
    for (auto &succ : after_block->get_succ_basic_blocks())
        succ->remove_pre_basic_block(after_block.get());
    after_block->get_succ_basic_blocks().clear();

    after_block->delete_instr(after_block->get_terminator_itr());
    BranchInst::create_br(new_before_block, after_block.get());

    vector<shared_ptr<BasicBlock>> update_phi_blocks;
    for (auto succ_block : new_after_block->get_succ_basic_blocks())
        update_phi_blocks.push_back(std::dynamic_pointer_cast<BasicBlock>(succ_block->shared_from_this()));
    utils::replace_phi(update_phi_blocks, to_new_operand);

    // 3. 处理 before_block 和 compare_block 的条件

    // before_block
    // 将 before_compare_end_var 替换为 before_compare_end_var - 8
    auto compare_right = before_compare_inst->get_operand(1);
    auto unroll_size_int = ConstantInt::get(unroll_size, before_block->get_module())->shared_from_this();

    auto new_compare_right = BinaryInst::create_sub(compare_right->shared_from_this(), unroll_size_int);
    before_block->insert_before(std::dynamic_pointer_cast<Instruction>(before_compare_inst->shared_from_this()),
                                new_compare_right);
    new_compare_right->set_parent(before_block.get());

    compare_right->remove_use(before_compare_inst);
    before_compare_inst->set_operand(1, new_compare_right->shared_from_this());

    // compare_block
    // 将 compare_right 替换为 compare_right - 8
    for (auto &inst : compare_block->get_instructions())
        if (inst->is_cmp()) {
            auto cmp_inst = dynamic_cast<CmpInst *>(inst.get());
            auto cmp_right = cmp_inst->get_operand(1);
            cmp_right->remove_use(cmp_inst);
            cmp_inst->set_operand(1, new_compare_right->shared_from_this());
        }

    // 3. 替换结束值、初始值
    vector<Instruction *> all_base_phi, all_after_phi;
    for (auto inst : base_block->get_instructions())
        if (inst->is_phi())
            all_base_phi.push_back(inst.get());
    for (auto inst : after_block->get_instructions())
        if (inst->is_phi())
            all_after_phi.push_back(inst.get());

    LOG_DEBUG << "all_base_phi: ";
    for (auto inst : all_base_phi)
        LOG_DEBUG << inst->print();
    LOG_DEBUG << "all_after_phi: ";
    for (auto inst : all_after_phi)
        LOG_DEBUG << inst->print();

    assert(all_base_phi.size() <= all_after_phi.size());

    // 用 new_after_block 中的 phi 替换 after_block 中的 phi 出现的地方
    for (auto after_phi : all_after_phi) {
        auto new_after_inst = to_new_operand[after_phi];
        assert(new_after_inst != nullptr);
        after_phi->replace_all_use_with(new_after_inst->shared_from_this());
    }

    // 将 after_block 中的非 phi 指令替换后删除
    for (auto inst_iter = after_block->get_instructions().begin();
         inst_iter != after_block->get_instructions().end();) {
        auto inst = *inst_iter;
        if (not inst->is_phi() and not inst->is_br()) {
            LOG_DEBUG << "delete inst: " << inst->print();
            auto new_inst = std::dynamic_pointer_cast<Instruction>(to_new_operand[inst.get()]->shared_from_this());
            inst->replace_all_use_with(new_inst);
            inst_iter = after_block->delete_instr(inst_iter);
        } else
            inst_iter++;
    }

    // 用 after_block 中的 phi 替换 new_base_block 中的 phi 来自 new_pre_block 的参数
    for (int i = 0; i < all_base_phi.size(); i++) {
        auto after_phi = all_after_phi[i];
        auto new_base_phi = dynamic_cast<Instruction *>(to_new_operand[all_base_phi[i]]);
        assert(new_base_phi != nullptr);

        int pair_num = new_base_phi->get_num_operand() / 2;
        for (int j = 0; j < pair_num; j++) {
            auto from_var = new_base_phi->get_operand(j * 2);
            auto from_bb = new_base_phi->get_operand(j * 2 + 1);
            if (from_bb.get() == new_pre_block.get()) {
                from_var->remove_use(new_base_phi);
                new_base_phi->set_operand(j * 2, after_phi->shared_from_this());
            }
        }
    }

    // 5. 对 base_block 循环展开
    map<Value *, Value *> phi_to_compare_block_value;
    for (auto phi : all_base_phi) {
        int pair_num = phi->get_num_operand() / 2;
        for (int i = 0; i < pair_num; i++) {
            auto value = phi->get_operand(i * 2).get();
            auto from_bb = phi->get_operand(i * 2 + 1).get();
            if (from_bb == compare_block.get())
                phi_to_compare_block_value[phi] = value;
        }
    }
    unroll_base_block(loop, unroll_base_times, phi_to_compare_block_value);
}

void LoopUnrolling::run() {
    LOG_INFO << "running pass loop-unrolling";
    _loop_info.run();
    module = m_;

    // 对内层循环展开
    auto all_loop = _loop_info.get_loops();

    int max_const_new_times = 105;
    int unroll_base_times = 4;
    int max_enable_new_instruction_count = 20;

    for (auto loop : all_loop) {
        // TODO: 只支持一个基本块的展开
        if (loop.get_blocks().size() > 2)
            continue;
        if (loop.get_sub_loops().size() > 0)
            continue;

        // 必须有归纳变量
        auto all_ind_var = loop.get_ind_vars();
        if (all_ind_var.size() == 0)
            continue;

        auto ind_var = all_ind_var.begin()->get();

        auto initial_val = ind_var->initial_val;
        auto final_val = ind_var->final_val;
        auto step_val = ind_var->step_val;

        auto initial_const = dynamic_cast<ConstantInt *>(initial_val.get());
        auto final_const = dynamic_cast<ConstantInt *>(final_val.get());
        auto step_const = dynamic_cast<ConstantInt *>(step_val.get());

        auto pre_block = loop.get_preheader();
        auto base_block = loop.get_base();
        auto compare_block = loop.get_latch();

        if (base_block->get_instructions().size() > 10)
            continue;

        // 只支持常量步长的循环展开
        if (step_const == nullptr)
            continue;

        // TODO: 暂时只支持常数
        if (initial_const != nullptr and final_const != nullptr) {
            int initial_int = initial_const->get_value();
            int final_int = final_const->get_value();
            int step_int = step_const->get_value();

            auto cmp_op = ind_var->predicate;
            int recurrence_times;
            if (cmp_op == (CmpOp)CmpOp::LT)
                recurrence_times = (final_int - initial_int) / step_int;
            else if (cmp_op == (CmpOp)CmpOp::LE)
                recurrence_times = (final_int - initial_int) / step_int + 1;
            else if (cmp_op == (CmpOp)CmpOp::GT)
                recurrence_times = (initial_int - final_int) / step_int;
            else if (cmp_op == (CmpOp)CmpOp::GE)
                recurrence_times = (initial_int - final_int) / step_int + 1;
            else
                continue;

            if (recurrence_times <= 0)
                continue;

            LOG_DEBUG << "recurrence_times: " << recurrence_times;

            vector<PhiInst *> phi_inst;
            vector<Instruction *> calculate_inst;
            for (auto inst : base_block->get_instructions()) {
                if (inst->is_phi())
                    phi_inst.push_back(static_cast<PhiInst *>(inst.get()));
                else if (not inst->is_br())
                    calculate_inst.push_back(inst.get());
            }

            map<Value *, Value *> phi_to_pre_value, phi_to_compare_block_value;
            map<Value *, int> phi_to_pre_index;

            for (auto phi : phi_inst) {
                int pair_num = phi->get_num_operand() / 2;
                for (int i = 0; i < pair_num; i++) {
                    auto value = phi->get_operand(i * 2).get();
                    auto from_bb = phi->get_operand(i * 2 + 1).get();
                    if (from_bb == pre_block.get()) {
                        phi_to_pre_value[phi] = value;
                        phi_to_pre_index[phi] = i * 2;
                    } else if (from_bb == compare_block.get())
                        phi_to_compare_block_value[phi] = value;
                }
            }

            assert(unroll_base_times < max_enable_new_instruction_count);

            bool skip_loop = false;
            if (recurrence_times < max_const_new_times)
                skip_loop = true;

            LOG_INFO << "unrolling loop: " << base_block->get_name();
            LOG_DEBUG << "initial_val: " << initial_const->get_value() << " final_val: " << final_const->get_value()
                      << " step_val: " << step_const->get_value() << " op: " << cmp_op;

            if (skip_loop)
                unroll_pre_block(
                    loop, recurrence_times, phi_to_pre_value, phi_to_pre_index, phi_to_compare_block_value, skip_loop);
            else {
                int unroll_pre_times = recurrence_times % unroll_base_times;

                unroll_pre_block(
                    loop, unroll_pre_times, phi_to_pre_value, phi_to_pre_index, phi_to_compare_block_value, skip_loop);
                unroll_base_block(loop, unroll_base_times, phi_to_compare_block_value);
            }
        } else {
            clone_and_new_loop(loop, unroll_base_times);
        }
    }
}
