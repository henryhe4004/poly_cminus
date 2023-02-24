#include "utils.hh"

namespace utils {
std::shared_ptr<Value> to_i32(std::shared_ptr<Value> v, Module *m) {
    auto i32_type = Type::get_int32_type(m);
    auto fp_type = Type::get_float_type(m);
    if (v->get_type() == i32_type)
        return v;
    if (auto v_c = get_const_float_val(v.get()); v_c.has_value())
        return ConstantInt::get(static_cast<int>(v_c.value()), m);
    return SiToFpInst::create_sitofp(v, fp_type, nullptr);
}

std::shared_ptr<Value> add_i32(std::shared_ptr<Value> v1, std::shared_ptr<Value> v2, Module *m) {
    auto i32_type = Type::get_int32_type(m);
    if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) {
        exit(204);
        LOG_ERROR << "both ops should be i32 type";
        return nullptr;
    }
    auto v1_c = get_const_int_val(v1.get());
    auto v2_c = get_const_int_val(v2.get());
    if (v1_c.has_value() && v2_c.has_value())
        return ConstantInt::get(v1_c.value() + v2_c.value(), m);
    // if (v1_c.has_value() && v1_c.value() == 0)
    //     return v2;
    // if (v2_c.has_value() && v2_c.value() == 0)
    //     return v1;
    return BinaryInst::create(i32_type, Instruction::add, v1, v2, nullptr);
}

std::shared_ptr<Value> sub_i32(std::shared_ptr<Value> v1, std::shared_ptr<Value> v2, Module *m) {
    auto i32_type = Type::get_int32_type(m);
    if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) {
        exit(205);
        LOG_ERROR << "both ops should be i32 type";
        return nullptr;
    }
    auto v1_c = get_const_int_val(v1.get());
    auto v2_c = get_const_int_val(v2.get());
    if (v1_c.has_value() && v2_c.has_value())
        return ConstantInt::get(v1_c.value() - v2_c.value(), m);
    // if (v2_c.has_value() && v2_c.value() == 0)
    //     return v1;
    return BinaryInst::create(i32_type, Instruction::sub, v1, v2, nullptr);
}

std::shared_ptr<Value> mul_i32(std::shared_ptr<Value> v1, std::shared_ptr<Value> v2, Module *m) {
    auto i32_type = Type::get_int32_type(m);
    if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) {
        exit(206);
        LOG_ERROR << "both ops should be i32 type";
        return nullptr;
    }
    auto v1_c = get_const_int_val(v1.get());
    auto v2_c = get_const_int_val(v2.get());
    if (v1_c.has_value() && v2_c.has_value())
        return ConstantInt::get(v1_c.value() * v2_c.value(), m);
    // if (v1_c.has_value() && v1_c.value() == 1)
    //     return v2;
    // if (v2_c.has_value() && v2_c.value() == 1)
    //     return v1;
    return BinaryInst::create(i32_type, Instruction::mul, v1, v2, nullptr);
}

std::shared_ptr<Value> add_i32(std::shared_ptr<Value> v1,
                               std::shared_ptr<Value> v2,
                               Module *m,
                               Instruction::self_iterator *insert_point) {
    auto i32_type = Type::get_int32_type(m);
    if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) {
        // exit(204);
        LOG_ERROR << "both ops should be i32 type";
        exit_if(true, UTILS_CALCULATE_TYPE_ERROR);
        return nullptr;
    }
    auto v1_c = get_const_int_val(v1.get());
    auto v2_c = get_const_int_val(v2.get());
    if (v1_c.has_value() && v2_c.has_value())
        return ConstantInt::get(v1_c.value() + v2_c.value(), m);
    if (v1_c == 0)
        return v2;
    if (v2_c == 0)
        return v1;
    return BinaryInst::create(i32_type, Instruction::add, v1, v2, insert_point);
}

std::shared_ptr<Value> sub_i32(std::shared_ptr<Value> v1,
                               std::shared_ptr<Value> v2,
                               Module *m,
                               Instruction::self_iterator *insert_point) {
    auto i32_type = Type::get_int32_type(m);
    if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) {
        // exit(204);
        LOG_ERROR << "both ops should be i32 type";
        exit_if(true, UTILS_CALCULATE_TYPE_ERROR);
        return nullptr;
    }
    auto v1_c = get_const_int_val(v1.get());
    auto v2_c = get_const_int_val(v2.get());
    if (v1_c.has_value() && v2_c.has_value())
        return ConstantInt::get(v1_c.value() - v2_c.value(), m);
    if (v2_c == 0)
        return v1;
    return BinaryInst::create(i32_type, Instruction::sub, v1, v2, insert_point);
}

std::shared_ptr<Value> mul_i32(std::shared_ptr<Value> v1,
                               std::shared_ptr<Value> v2,
                               Module *m,
                               Instruction::self_iterator *insert_point) {
    auto i32_type = Type::get_int32_type(m);
    if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) {
        // exit(204);
        LOG_ERROR << "both ops should be i32 type";
        exit_if(true, UTILS_CALCULATE_TYPE_ERROR);
        return nullptr;
    }
    auto v1_c = get_const_int_val(v1.get());
    auto v2_c = get_const_int_val(v2.get());
    if (v1_c.has_value() && v2_c.has_value())
        return ConstantInt::get(v1_c.value() * v2_c.value(), m);
    if (v1_c == 1)
        return v2;
    if (v2_c == 1)
        return v1;
    if (v1_c == 0 || v2_c == 0)
        return ConstantInt::get(0, m);
    return BinaryInst::create(i32_type, Instruction::mul, v1, v2, insert_point);
}

std::shared_ptr<Value> sdiv_i32(std::shared_ptr<Value> v1,
                                std::shared_ptr<Value> v2,
                                Module *m,
                                Instruction::self_iterator *insert_point) {
    auto i32_type = Type::get_int32_type(m);
    if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) {
        // exit(204);
        LOG_ERROR << "both ops should be i32 type";
        exit_if(true, UTILS_CALCULATE_TYPE_ERROR);
        return nullptr;
    }
    auto v1_c = get_const_int_val(v1.get());
    auto v2_c = get_const_int_val(v2.get());
    if (v1_c.has_value() && v2_c.has_value())
        return ConstantInt::get(v1_c.value() / v2_c.value(), m);
    if (v2_c == 1)
        return v1;
    if (v1_c == 0)
        return ConstantInt::get(0, m);
    return BinaryInst::create(i32_type, Instruction::sdiv, v1, v2, insert_point);
}

std::shared_ptr<Value> to_operand(Value *val, std::map<Value *, Value *> &to_new_operand) {
    auto it = to_new_operand.find(val);
    Value *result = val;
    if (it != to_new_operand.end())
        result = it->second;
    return result->shared_from_this();
}

// clone all_inst，加在 bb 后面，尝试用 to_new_operand 替换操作数
std::vector<Instruction *> clone_instructions(std::vector<Instruction *> all_inst,
                                              BasicBlock *bb,
                                              std::map<Value *, Value *> &to_new_operand) {
    std::vector<Instruction *> new_instrustions;

    for (auto inst : all_inst) {
        std::shared_ptr<Instruction> new_inst;
        // if (inst->is_ret()) {
        // } else
        if (inst->is_br()) {
            if (inst->get_num_operand() == 1) {
                auto target_bb =
                    std::dynamic_pointer_cast<BasicBlock>(to_operand(inst->get_operand(0).get(), to_new_operand));
                assert(target_bb != nullptr);
                new_inst = BranchInst::create_br(target_bb, bb);
            } else {
                auto cond = to_operand(inst->get_operand(0).get(), to_new_operand);
                auto bb_true =
                    std::dynamic_pointer_cast<BasicBlock>(to_operand(inst->get_operand(1).get(), to_new_operand));
                auto bb_false =
                    std::dynamic_pointer_cast<BasicBlock>(to_operand(inst->get_operand(2).get(), to_new_operand));
                assert(bb_true != nullptr);
                assert(bb_false != nullptr);
                new_inst = BranchInst::create_cond_br(cond, bb_true, bb_false, bb);
            }
        } else if (inst->isBinary())
            new_inst = BinaryInst::create(inst->get_type(),
                                          inst->get_instr_type(),
                                          to_operand(inst->get_operand(0).get(), to_new_operand),
                                          to_operand(inst->get_operand(1).get(), to_new_operand),
                                          bb);
        else if (inst->is_cmp())
            new_inst = CmpInst::create_cmp(static_cast<CmpInst *>(inst)->get_cmp_op(),
                                           to_operand(inst->get_operand(0).get(), to_new_operand),
                                           to_operand(inst->get_operand(1).get(), to_new_operand),
                                           bb);
        else if (inst->is_fcmp())
            new_inst = FCmpInst::create_fcmp(static_cast<FCmpInst *>(inst)->get_cmp_op(),
                                             to_operand(inst->get_operand(0).get(), to_new_operand),
                                             to_operand(inst->get_operand(1).get(), to_new_operand),
                                             bb);
        else if (inst->is_call()) {
            auto func = inst->get_operand(0).get();
            auto new_func = std::static_pointer_cast<Function>(to_operand(func, to_new_operand));

            std::vector<std::shared_ptr<Value>> new_arg_list;
            for (int i = 1; i < inst->get_num_operand(); i++)
                new_arg_list.push_back(to_operand(inst->get_operand(i).get(), to_new_operand));

            new_inst = CallInst::create(new_func, new_arg_list, bb);
        } else if (inst->is_gep()) {
            auto val = inst->get_operand(0).get();
            auto new_val = to_operand(val, to_new_operand);

            std::vector<std::shared_ptr<Value>> all_new_index;
            for (int i = 1; i < inst->get_num_operand(); i++)
                all_new_index.push_back(to_operand(inst->get_operand(i).get(), to_new_operand));
            new_inst = GetElementPtrInst::create_gep(new_val, all_new_index, bb);
        } else if (inst->is_store())
            new_inst = StoreInst::create_store(to_operand(inst->get_operand(0).get(), to_new_operand),
                                               to_operand(inst->get_operand(1).get(), to_new_operand),
                                               bb);
        else if (inst->is_load())
            new_inst =
                LoadInst::create_load(inst->get_type(), to_operand(inst->get_operand(0).get(), to_new_operand), bb);
        else if (inst->is_alloca())
            new_inst = AllocaInst::create_alloca(static_cast<AllocaInst *>(inst)->get_alloca_type(), bb);
        else if (inst->is_zext())
            new_inst = ZextInst::create_zext(to_operand(inst->get_operand(0).get(), to_new_operand),
                                             static_cast<ZextInst *>(inst)->get_dest_type(),
                                             bb);
        else if (inst->is_si2fp())
            new_inst = SiToFpInst::create_sitofp(to_operand(inst->get_operand(0).get(), to_new_operand),
                                                 static_cast<SiToFpInst *>(inst)->get_dest_type(),
                                                 bb);
        else if (inst->is_fp2si())
            new_inst = FpToSiInst::create_fptosi(to_operand(inst->get_operand(0).get(), to_new_operand),
                                                 static_cast<FpToSiInst *>(inst)->get_dest_type(),
                                                 bb);
        else if (inst->is_phi()) {
            auto phi = static_cast<PhiInst *>(inst);
            auto operands = phi->get_operands();
            std::vector<std::shared_ptr<Value>> new_operands;
            for (auto operand : operands)
                new_operands.push_back(operand.get()->shared_from_this());
            auto new_phi_inst = PhiInst::create_phi(phi->get_type(), bb);
            int pair_num = phi->get_num_operand() / 2;

            for (int i = 0; i < pair_num; i++) {
                auto new_bb =
                    std::dynamic_pointer_cast<BasicBlock>(to_operand(new_operands[i * 2 + 1].get(), to_new_operand));
                assert(new_bb != nullptr);
                new_phi_inst->add_phi_pair_operand(new_operands[i * 2], new_bb);
            }
            new_inst = new_phi_inst;
        } else if (inst->is_inttoptr()) {
            auto new_val = to_operand(inst->get_operand(0).get(), to_new_operand);
            new_inst = IntToPtrInst::create_inttoptr(new_val, inst->get_type(), bb);
        } else if (inst->is_ptrtoint()) {
            auto new_val = to_operand(inst->get_operand(0).get(), to_new_operand);
            new_inst = PtrToIntInst::create_ptrtoint(new_val, bb);
        } else {
            // TODO: 其他指令
            LOG_ERROR << "not support instruction: " << inst->print();
            exit(123);
        }

        new_instrustions.push_back(new_inst.get());
        to_new_operand[inst] = new_inst.get();
    }
    return new_instrustions;
}

void replace_phi(std::vector<std::shared_ptr<BasicBlock>> new_blocks, std::map<Value *, Value *> &to_new_operand) {
    // 替换 phi 的 operand
    std::set<BasicBlock *> need_update_phi_blocks;
    for (auto block : new_blocks) {
        // auto succ_blocks = block->get_succ_basic_blocks();
        // for (auto succ_block : succ_blocks)
        //     need_update_phi_blocks.insert(succ_block);
        need_update_phi_blocks.insert(block.get());
    }
    for (auto block : need_update_phi_blocks) {
        for (auto inst : block->get_instructions()) {
            auto phi = dynamic_cast<PhiInst *>(inst.get());
            if (phi == nullptr)
                continue;
            int pair_num = phi->get_num_operand() / 2;
            std::map<Value *, Value *> bb_to_value;
            for (int i = 0; i < pair_num; i++) {
                auto bb = phi->get_operand(i * 2 + 1).get();
                auto value = phi->get_operand(i * 2).get();
                bb_to_value[bb] = value;

                bb_to_value[to_operand(bb, to_new_operand).get()] = to_operand(value, to_new_operand).get();
            }
            // clear all bb_to_value[bb]
            for (int i = 0; i < pair_num; i++)
                auto bb = phi->get_operand(i * 2 + 1).get();

            auto pre_blocks = phi->get_parent()->get_pre_basic_blocks();

            phi->remove_operands(0, pair_num * 2 - 1);
            for (auto [bb, value] : bb_to_value)
                if (std::find(pre_blocks.begin(), pre_blocks.end(), bb) != pre_blocks.end())
                    phi->add_phi_pair_operand(value->shared_from_this(),
                                              std::dynamic_pointer_cast<BasicBlock>(bb->shared_from_this()));
        }
    }
}

void replace_phi_indexbb(std::map<Instruction *, std::map<BasicBlock *, Value *>> &phi_index_bb_map) {
    for (auto [phi, bb_value_map] : phi_index_bb_map)
        for (auto [target_bb, target_value] : bb_value_map) {
            int pair_num = phi->get_num_operand() / 2;
            for (int i = 0; i < pair_num; i++) {
                auto bb = phi->get_operand(i * 2 + 1).get();
                auto value = phi->get_operand(i * 2).get();
                if (bb == target_bb) {
                    value->remove_use(phi);
                    phi->set_operand(i * 2, target_value->shared_from_this());
                }
            }
        }
}

void clone_blocks(std::vector<BasicBlock *> blocks,
                  std::vector<std::shared_ptr<BasicBlock>> new_blocks,
                  std::map<Value *, Value *> &to_new_operand) {
    for (int i = 0; i < blocks.size(); i++) {
        auto block = blocks[i];
        auto new_block = new_blocks[i];
        std::vector<Instruction *> block_inst;
        for (auto inst : block->get_instructions())
            block_inst.push_back(inst.get());
        auto new_instructions = utils::clone_instructions(block_inst, new_block.get(), to_new_operand);
    }
}

std::shared_ptr<BasicBlock> init_create_block(BasicBlock *block, std::string suffix) {
    auto func = block->get_parent();
    auto module = func->get_parent();
    return BasicBlock::create(module, block->get_name().substr(6) + suffix, func);
}

}  // namespace utils
