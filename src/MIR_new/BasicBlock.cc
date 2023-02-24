#include "BasicBlock.h"

#include "Function.h"
#include "IRprinter.h"
#include "Module.h"

#include <cassert>

BasicBlock::BasicBlock(Module *m, const std::string &name = "", Function *parent = nullptr)
    : Value(Type::get_label_type(m), name), parent_(parent) {
    assert(parent && "currently parent should not be nullptr");
}

BasicBlock::BasicBlock(Module *m,
                       const std::string &name,
                       std::list<std::shared_ptr<BasicBlock>>::iterator insert_before)
    : Value(Type::get_label_type(m), name), parent_((*insert_before)->get_parent()) {}

std::shared_ptr<BasicBlock> BasicBlock::create(Module *m, const std::string &name, Function *parent) {
    auto prefix = name.empty() ? "" : "label_";
    auto ptr = std::shared_ptr<BasicBlock>(new BasicBlock(m, prefix + name, parent));
    parent->add_basic_block(ptr);
    return ptr;
}

std::shared_ptr<BasicBlock> BasicBlock::create(Module *m,
                                               const std::string &name,
                                               std::list<std::shared_ptr<BasicBlock>>::iterator insert_before) {
    auto prefix = name.empty() ? "" : "label_";
    auto ptr = std::shared_ptr<BasicBlock>(new BasicBlock(m, prefix + name, insert_before));
    (*insert_before)->get_parent()->get_basic_blocks().insert(insert_before, ptr);
    return ptr;
}

Module *BasicBlock::get_module() { return get_parent()->get_parent(); }

void BasicBlock::add_instruction(std::shared_ptr<Instruction> instr) { instr_list_.push_back(instr); }

void BasicBlock::add_instr_begin(std::shared_ptr<Instruction> instr) { instr_list_.push_front(instr); }

void BasicBlock::delete_instr(std::shared_ptr<Instruction> instr) {
    instr_list_.erase(find_instr(instr));
    instr->remove_use_of_ops();  // 可能存在的问题：访问已释放的空间
}

inst_list_iterator_type BasicBlock::delete_instr(inst_list_iterator_type it) {
    (*it)->remove_use_of_ops();
    return instr_list_.erase(it);
}

const std::shared_ptr<Instruction> BasicBlock::get_terminator() const {
    if (instr_list_.empty()) {
        return nullptr;
    }
    switch (instr_list_.back()->get_instr_type()) {
        case Instruction::ret:
        case Instruction::br:
        case Instruction::switch1:
            return instr_list_.back();
        default:
            return nullptr;
    }
}

std::pair<inst_list_iterator_type, inst_list_iterator_type> BasicBlock::get_phis() {
    for (auto it = instr_list_.begin(); it != instr_list_.end(); ++it)
        if (not(*it)->is_phi())
            return {instr_list_.begin(), it};
    return {instr_list_.begin(), instr_list_.end()};
}

std::string BasicBlock::print() {
    std::string bb_ir;
    bb_ir += this->get_name();
    bb_ir += ":";
    // print prebb
    if (!this->get_pre_basic_blocks().empty()) {
        bb_ir += "                                                ; preds = ";
    }
    for (auto bb : this->get_pre_basic_blocks()) {
        if (bb != *this->get_pre_basic_blocks().begin())
            bb_ir += ", ";
        bb_ir += print_as_op(bb, false);
    }

    // print prebb
    if (!this->get_parent()) {
        bb_ir += "\n";
        bb_ir += "; Error: Block without parent!";
    }
    bb_ir += "\n";
    for (auto instr : this->get_instructions()) {
        bb_ir += "  ";
        bb_ir += instr->print();
        bb_ir += "\n";
    }

    return bb_ir;
}

void BasicBlock::remove_pre_basic_block(BasicBlock *bb) {
    pre_bbs_.remove(bb);
    // WARN: 可能会有临时移除 bb 的情况，因此不应该默认重写 phi 指令
    // for (auto inst : get_instructions()) {
    //     if (not inst->is_phi())
    //         break;
    //     auto phiinst = std::dynamic_pointer_cast<PhiInst>(inst);
    //     phiinst->remove_bb(std::dynamic_pointer_cast<BasicBlock>(bb->shared_from_this()));
    // }
}

void BasicBlock::remove_pre_basic_block_phi(BasicBlock *bb) {
    pre_bbs_.remove(bb);
    // WARN: 可能会有临时移除 bb 的情况，因此不应该默认重写 phi 指令
    for (auto inst : get_instructions()) {
        if (not inst->is_phi())
            break;
        auto phiinst = std::dynamic_pointer_cast<PhiInst>(inst);
        phiinst->remove_bb(std::dynamic_pointer_cast<BasicBlock>(bb->shared_from_this()));
    }
}

void BasicBlock::remove_bb_in_phi(std::shared_ptr<BasicBlock> bb) {
    for (auto inst : get_instructions()) {
        if (not inst->is_phi())
            break;
        auto phiinst = std::dynamic_pointer_cast<PhiInst>(inst);
        phiinst->remove_bb(bb);
    }
}
