#include "Instruction.h"

#include "BasicBlock.h"
#include "Function.h"
#include "IRprinter.h"
#include "Module.h"
#include "Type.h"

#include <algorithm>
#include <cassert>
#include <vector>

Instruction::Instruction(Type *ty, OpID id, unsigned num_ops, BasicBlock *parent)
    : User(ty, "", num_ops), op_id_(id), num_ops_(num_ops), parent_(parent) {
    // parent_->add_instruction(this);
}

Instruction::Instruction(Type *ty, OpID id, unsigned num_ops, Instruction::self_iterator *insert_before)
    : User(ty, "", num_ops), op_id_(id), num_ops_(num_ops) {
    if (insert_before)
        parent_ = (**insert_before)->get_parent();
}

Function *Instruction::get_function() { return parent_->get_parent(); }

Module *Instruction::get_module() { return parent_->get_module(); }

BinaryInst::BinaryInst(Type *ty, OpID id, std::shared_ptr<Value> v1, std::shared_ptr<Value> v2, BasicBlock *bb)
    : BaseInst<BinaryInst>(ty, id, 2, bb) {
    set_operand(0, v1);
    set_operand(1, v2);
    // assertValid();
}

BinaryInst::BinaryInst(Type *ty,
                       OpID id,
                       std::shared_ptr<Value> v1,
                       std::shared_ptr<Value> v2,
                       Instruction::self_iterator *insert_before)
    : BaseInst<BinaryInst>(ty, id, 2, insert_before) {
    set_operand(0, v1);
    set_operand(1, v2);
    // assertValid();
}

std::shared_ptr<BinaryInst> BinaryInst::create_add(std::shared_ptr<Value> v1,
                                                   std::shared_ptr<Value> v2,
                                                   Instruction::self_iterator *insert_before) {
    return create(v1->get_type(), Instruction::add, v1, v2, insert_before);
}

std::shared_ptr<BinaryInst> BinaryInst::create_mul(std::shared_ptr<Value> v1,
                                                   std::shared_ptr<Value> v2,
                                                   Instruction::self_iterator *insert_before) {
    return create(v1->get_type(), Instruction::mul, v1, v2, insert_before);
}

void BinaryInst::assertValid() {
    if (!(get_operand(0)->get_type()->is_integer_type()))
        exit(210);
    if (!(get_operand(1)->get_type()->is_integer_type()))
        exit(211);
    if (!(static_cast<IntegerType *>(get_operand(0)->get_type())->get_num_bits() ==
          static_cast<IntegerType *>(get_operand(1)->get_type())->get_num_bits()))
        exit(212);
}

std::string BinaryInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(get_operand(0).get(), false);
    instr_ir += ", ";
    if (Type::is_eq_type(this->get_operand(0)->get_type(), this->get_operand(1)->get_type())) {
        instr_ir += print_as_op(get_operand(1).get(), false);
    } else {
        instr_ir += print_as_op(get_operand(1).get(), true);
    }
    if (op2_sh_num > 0) {
        if (op2_sh_type == shift_type_t::LSL)
            instr_ir += ", LSL ";
        else if (op2_sh_type == shift_type_t::LSR)
            instr_ir += ", LSR ";
        else if (op2_sh_type == shift_type_t::ASR)
            instr_ir += ", ASR ";
        instr_ir += std::to_string(op2_sh_num);
    }
    return instr_ir;
}

CmpInst::CmpInst(Type *ty, CmpOp op, std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs, BasicBlock *bb)
    : BaseInst<CmpInst>(ty, Instruction::cmp, 2, bb), cmp_op_(op) {
    set_operand(0, lhs);
    set_operand(1, rhs);
    // assertValid();
}

void CmpInst::assertValid() {
    if (!(get_operand(0)->get_type()->is_integer_type()))
        exit(213);
    if (!(get_operand(1)->get_type()->is_integer_type()))
        exit(214);
    if (!(static_cast<IntegerType *>(get_operand(0)->get_type())->get_num_bits() ==
          static_cast<IntegerType *>(get_operand(1)->get_type())->get_num_bits()))
        exit(215);
}

std::string CmpInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += print_cmp_type(this->cmp_op_);
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(get_operand(0).get(), false);
    instr_ir += ", ";
    if (Type::is_eq_type(this->get_operand(0)->get_type(), this->get_operand(1)->get_type())) {
        instr_ir += print_as_op(get_operand(1).get(), false);
    } else {
        instr_ir += print_as_op(get_operand(1).get(), true);
    }
    return instr_ir;
}

FCmpInst::FCmpInst(Type *ty, CmpOp op, std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs, BasicBlock *bb)
    : BaseInst<FCmpInst>(ty, Instruction::fcmp, 2, bb), cmp_op_(op) {
    set_operand(0, lhs);
    set_operand(1, rhs);
    // assertValid();
}

void FCmpInst::assert_valid() {
    if (!(get_operand(0)->get_type()->is_float_type()))
        exit(216);
    if (!(get_operand(1)->get_type()->is_float_type()))
        exit(217);
}

std::string FCmpInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += print_fcmp_type(this->cmp_op_);
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(get_operand(0).get(), false);
    instr_ir += ",";
    if (Type::is_eq_type(this->get_operand(0)->get_type(), this->get_operand(1)->get_type())) {
        instr_ir += print_as_op(get_operand(1).get(), false);
    } else {
        instr_ir += print_as_op(get_operand(1).get(), true);
    }
    return instr_ir;
}

SelectInst::SelectInst(std::shared_ptr<Value> test_bit,
                       std::shared_ptr<Value> t,
                       std::shared_ptr<Value> f,
                       BasicBlock *bb)
    : BaseInst<SelectInst>(t->get_type(), Instruction::select, 3, bb) {
    set_operand(0, test_bit);
    set_operand(1, t);
    set_operand(2, f);
}

SelectInst::SelectInst(std::shared_ptr<Value> test_bit,
                       std::shared_ptr<Value> t,
                       std::shared_ptr<Value> f,
                       Instruction::self_iterator *insert_before)
    : BaseInst<SelectInst>(t->get_type(), Instruction::select, 3, insert_before) {
    set_operand(0, test_bit);
    set_operand(1, t);
    set_operand(2, f);
}

std::string SelectInst::print() {
    std::string ir;
    ir += "%";
    ir += this->get_name();
    ir += " = ";
    ir += "select ";
    ir += print_as_op(get_operand(0).get(), true);
    ir += ", ";
    ir += print_as_op(get_operand(1).get(), true);
    ir += ", ";
    ir += print_as_op(get_operand(2).get(), true);
    return ir;
}

CallInst::CallInst(std::shared_ptr<Function> func, std::vector<std::shared_ptr<Value>> args, BasicBlock *bb)
    : BaseInst<CallInst>(func->get_return_type(), Instruction::call, args.size() + 1, bb) {
    if (!(func->get_num_of_args() == args.size()))
        exit(218);
    int num_ops = args.size() + 1;
    set_operand(0, std::weak_ptr<Value>(func));
    for (int i = 1; i < num_ops; i++) {
        set_operand(i, args[i - 1]);
    }
}

CallInst::CallInst(std::shared_ptr<Function> func,
                   std::vector<std::shared_ptr<Value>> args,
                   Instruction::self_iterator *insert_before)
    : BaseInst<CallInst>(func->get_return_type(), Instruction::call, args.size() + 1, insert_before) {
    if (!(func->get_num_of_args() == args.size()))
        exit(218);
    int num_ops = args.size() + 1;
    set_operand(0, std::weak_ptr<Value>(func));
    for (int i = 1; i < num_ops; i++) {
        set_operand(i, args[i - 1]);
    }
}

FunctionType *CallInst::get_function_type() const { return static_cast<FunctionType *>(get_operand(0)->get_type()); }
Function *CallInst::get_callee() { return dynamic_cast<Function *>(get_operand(0).get()); }
std::string CallInst::print() {
    std::string instr_ir;
    if (!this->is_void()) {
        instr_ir += "%";
        instr_ir += this->get_name();
        instr_ir += " = ";
    }
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_function_type()->get_return_type()->print();

    instr_ir += " ";
    if (!(dynamic_cast<Function *>(this->get_operand(0).get()) && "Wrong call operand function"))
        exit(219);
    instr_ir += print_as_op(get_operand(0).get(), false);
    instr_ir += "(";
    for (int i = 1; i < this->get_num_operand(); i++) {
        if (i > 1)
            instr_ir += ", ";
        instr_ir += this->get_operand(i)->get_type()->print();
        instr_ir += " ";
        instr_ir += print_as_op(get_operand(i).get(), false);
    }
    instr_ir += ")";
    return instr_ir;
}

BranchInst::BranchInst(std::shared_ptr<Value> cond,
                       std::shared_ptr<BasicBlock> if_true,
                       std::shared_ptr<BasicBlock> if_false,
                       BasicBlock *bb)
    : BaseInst<BranchInst>(Type::get_void_type(if_true->get_module()), Instruction::br, 3, bb) {
    set_operand(0, cond);
    // FIXME: cyclic dependency between BranchInst and BasicBlock
    set_operand(1, std::weak_ptr<Value>(if_true));
    set_operand(2, std::weak_ptr<Value>(if_false));
}

BranchInst::BranchInst(std::shared_ptr<BasicBlock> if_true, BasicBlock *bb)
    : BaseInst<BranchInst>(Type::get_void_type(if_true->get_module()), Instruction::br, 1, bb) {
    set_operand(0, std::weak_ptr<Value>(if_true));
}

std::shared_ptr<BranchInst> BranchInst::create_cond_br(std::shared_ptr<Value> cond,
                                                       std::shared_ptr<BasicBlock> if_true,
                                                       std::shared_ptr<BasicBlock> if_false,
                                                       BasicBlock *bb) {
    if_true->add_pre_basic_block(bb);
    if_false->add_pre_basic_block(bb);
    bb->add_succ_basic_block(if_false.get());
    bb->add_succ_basic_block(if_true.get());
    return create(cond, if_true, if_false, bb);
}

std::shared_ptr<BranchInst> BranchInst::create_br(std::shared_ptr<BasicBlock> if_true, BasicBlock *bb) {
    if_true->add_pre_basic_block(bb);
    bb->add_succ_basic_block(if_true.get());

    return create(if_true, bb);
}

bool BranchInst::is_cond_br() const { return get_num_operand() == 3; }

std::string BranchInst::print() {
    std::string instr_ir;
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    // instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += print_as_op(get_operand(0).get(), true);
    if (is_cond_br()) {
        instr_ir += ", ";
        instr_ir += print_as_op(get_operand(1).get(), true);
        instr_ir += ", ";
        instr_ir += print_as_op(get_operand(2).get(), true);
    }
    return instr_ir;
}

ReturnInst::ReturnInst(std::shared_ptr<Value> val, BasicBlock *bb)
    : BaseInst<ReturnInst>(Type::get_void_type(bb->get_module()), Instruction::ret, 1, bb) {
    set_operand(0, val);
}

ReturnInst::ReturnInst(BasicBlock *bb)
    : BaseInst<ReturnInst>(Type::get_void_type(bb->get_module()), Instruction::ret, 0, bb) {}

bool ReturnInst::is_void_ret() const { return get_num_operand() == 0; }

std::string ReturnInst::print() {
    std::string instr_ir;
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    if (!is_void_ret()) {
        instr_ir += this->get_operand(0)->get_type()->print();
        instr_ir += " ";
        instr_ir += print_as_op(get_operand(0).get(), false);
    } else {
        instr_ir += "void";
    }

    return instr_ir;
}

GetElementPtrInst::GetElementPtrInst(std::shared_ptr<Value> ptr,
                                     std::vector<std::shared_ptr<Value>> idxs,
                                     BasicBlock *bb)
    : BaseInst<GetElementPtrInst>(PointerType::get(get_element_type(ptr, idxs)),
                                  Instruction::getelementptr,
                                  1 + idxs.size(),
                                  bb) {
    set_operand(0, ptr);
    for (int i = 0; i < idxs.size(); i++) {
        set_operand(i + 1, idxs[i]);
    }
    element_ty_ = get_element_type(ptr, idxs);
}

GetElementPtrInst::GetElementPtrInst(std::shared_ptr<Value> ptr,
                                     std::vector<std::shared_ptr<Value>> idxs,
                                     Instruction::self_iterator *insert_before)
    : BaseInst<GetElementPtrInst>(PointerType::get(get_element_type(ptr, idxs)),
                                  Instruction::getelementptr,
                                  1 + idxs.size(),
                                  insert_before) {
    set_operand(0, ptr);
    for (int i = 0; i < idxs.size(); i++) {
        set_operand(i + 1, idxs[i]);
    }
    element_ty_ = get_element_type(ptr, idxs);
}

Type *GetElementPtrInst::get_element_type(std::shared_ptr<Value> ptr, std::vector<std::shared_ptr<Value>> idxs) {
    Type *ty = ptr->get_type()->get_pointer_element_type();
    if (!("GetElementPtrInst ptr is wrong type" &&
          (ty->is_array_type() || ty->is_integer_type() || ty->is_float_type())))
        exit(ty->get_type_id());
    if (ty->is_array_type()) {
        ArrayType *arr_ty = static_cast<ArrayType *>(ty);
        for (int i = 1; i < idxs.size(); i++) {
            ty = arr_ty->get_element_type();
            if (i < idxs.size() - 1) {
                if (!(ty->is_array_type() && "Index error!"))
                    exit(221);
            }
            if (ty->is_array_type()) {
                arr_ty = static_cast<ArrayType *>(ty);
            }
        }
    }
    return ty;
}

Type *GetElementPtrInst::get_element_type() const { return element_ty_; }

std::string GetElementPtrInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    if (!(this->get_operand(0)->get_type()->is_pointer_type()))
        exit(222);
    instr_ir += this->get_operand(0)->get_type()->get_pointer_element_type()->print();
    instr_ir += ", ";
    for (int i = 0; i < this->get_num_operand(); i++) {
        if (i > 0)
            instr_ir += ", ";
        instr_ir += this->get_operand(i)->get_type()->print();
        instr_ir += " ";
        if (i == 0) {
            auto glo = dynamic_cast<GlobalVariable *>(this->get_operand(i).get());
            if (glo && glo->get_type()->get_pointer_element_type() != glo->get_init()->get_type()) {
                instr_ir += "bitcast (";
                instr_ir += glo->get_init()->get_type()->print();
                instr_ir += "* ";
                instr_ir += print_as_op(get_operand(i).get(), false);
                instr_ir += " to ";
                instr_ir += glo->get_type()->print();
                instr_ir += ")";
            } else
                instr_ir += print_as_op(get_operand(i).get(), false);
        } else
            instr_ir += print_as_op(get_operand(i).get(), false);
    }
    return instr_ir;
}

StoreInst::StoreInst(std::shared_ptr<Value> val, std::shared_ptr<Value> ptr, BasicBlock *bb)
    : BaseInst<StoreInst>(Type::get_void_type(bb->get_module()), Instruction::store, 2, bb) {
    set_operand(0, val);
    set_operand(1, ptr);
}

StoreInst::StoreInst(std::shared_ptr<Value> val, std::shared_ptr<Value> ptr, self_iterator *insert_before)
    : BaseInst<StoreInst>(Type::get_void_type((**insert_before)->get_module()), Instruction::store, 2, insert_before) {
    set_operand(0, val);
    set_operand(1, ptr);
}

std::string StoreInst::print() {
    std::string instr_ir;
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(get_operand(0).get(), false);
    instr_ir += ", ";
    instr_ir += print_as_op(get_operand(1).get(), true);
    if (get_num_operand() > 2) {
        instr_ir += ", ";
        instr_ir += print_as_op(get_operand(2).get(), true);
        if (op2_sh_num > 0) {
            if (op2_sh_type == shift_type_t::LSL)
                instr_ir += ", LSL ";
            else if (op2_sh_type == shift_type_t::LSR)
                instr_ir += ", LSR ";
            else if (op2_sh_type == shift_type_t::ASR)
                instr_ir += ", ASR ";
            instr_ir += std::to_string(op2_sh_num);
        }
    }
    return instr_ir;
}

LoadInst::LoadInst(Type *ty, std::shared_ptr<Value> ptr, BasicBlock *bb)
    : BaseInst<LoadInst>(ty, Instruction::load, 1, bb) {
    if (!(ptr->get_type()->is_pointer_type()))
        exit(223);
    if (!(ty == static_cast<PointerType *>(ptr->get_type())->get_element_type()))
        exit(224);
    set_operand(0, ptr);
}

LoadInst::LoadInst(Type *ty, std::shared_ptr<Value> ptr, self_iterator *insert_before)
    : BaseInst<LoadInst>(ty, Instruction::load, 1, insert_before) {
    if (!(ptr->get_type()->is_pointer_type()))
        exit(225);
    if (!(ty == static_cast<PointerType *>(ptr->get_type())->get_element_type()))
        exit(226);
    set_operand(0, ptr);
}

// std::shared_ptr<LoadInst> LoadInst::create_load(Type *ty, std::shared_ptr<Value> ptr) {
//     return std::shared_ptr<LoadInst>(new LoadInst(ty, ptr));
// }

Type *LoadInst::get_load_type() const {
    return static_cast<PointerType *>(get_operand(0)->get_type())->get_element_type();
}

std::string LoadInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    if (!(this->get_operand(0)->get_type()->is_pointer_type()))
        instr_ir += this->get_operand(0)->get_type()->print();
    else
        instr_ir += this->get_operand(0)->get_type()->get_pointer_element_type()->print();
    instr_ir += ",";
    instr_ir += " ";
    instr_ir += print_as_op(get_operand(0).get(), true);
    if (get_num_operand() > 1) {
        instr_ir += ", ";
        instr_ir += print_as_op(get_operand(1).get(), true);
        if (op2_sh_num > 0) {
            if (op2_sh_type == shift_type_t::LSL)
                instr_ir += ", LSL ";
            else if (op2_sh_type == shift_type_t::LSR)
                instr_ir += ", LSR ";
            else if (op2_sh_type == shift_type_t::ASR)
                instr_ir += ", ASR ";
            instr_ir += std::to_string(op2_sh_num);
        }
    }
    return instr_ir;
}

AllocaInst::AllocaInst(Type *ty, BasicBlock *bb)
    : BaseInst<AllocaInst>(PointerType::get(ty), Instruction::alloca, 0, bb), alloca_ty_(ty) {}

AllocaInst::AllocaInst(Type *ty, self_iterator *insert_before)
    : BaseInst<AllocaInst>(PointerType::get(ty), Instruction::alloca, 0, insert_before), alloca_ty_(ty) {}
Type *AllocaInst::get_alloca_type() const { return alloca_ty_; }

std::string AllocaInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += get_alloca_type()->print();
    return instr_ir;
}

PtrToIntInst::PtrToIntInst(std::shared_ptr<Value> value, BasicBlock *bb)
    : BaseInst<PtrToIntInst>(Type::get_ptrtoint_type(value->get_type()->get_module()), Instruction::ptrtoint, 1, bb) {
    // pretty shitty workaround
    set_operand(0, value);
}

PtrToIntInst::PtrToIntInst(std::shared_ptr<Value> value, Instruction::self_iterator *insert_before)
    : BaseInst<PtrToIntInst>(Type::get_ptrtoint_type(value->get_type()->get_module()),
                             Instruction::ptrtoint,
                             1,
                             insert_before) {
    set_operand(0, value);
}

IntToPtrInst::IntToPtrInst(std::shared_ptr<Value> value, Type *to, BasicBlock *bb)
    : BaseInst<IntToPtrInst>(to, Instruction::inttoptr, 1, bb) {
    set_operand(0, value);
}

IntToPtrInst::IntToPtrInst(std::shared_ptr<Value> value, Type *to, Instruction::self_iterator *insert_before)
    : BaseInst<IntToPtrInst>(to, Instruction::inttoptr, 1, insert_before) {
    set_operand(0, value);
}

std::string PtrToIntInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";

    instr_ir += get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += print_as_op(get_operand(0).get(), true);
    instr_ir += " to ";
    instr_ir += this->get_type()->print();
    return instr_ir;
}

std::string IntToPtrInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";

    instr_ir += get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += get_operand(0)->get_type()->print();
    instr_ir += " %";
    instr_ir += get_operand(0)->get_name();
    instr_ir += " to ";
    instr_ir += this->get_type()->print();
    return instr_ir;
}

ZextInst::ZextInst(OpID op, std::shared_ptr<Value> val, Type *ty, BasicBlock *bb)
    : BaseInst<ZextInst>(ty, op, 1, bb), dest_ty_(ty) {
    set_operand(0, val);
}

ZextInst::ZextInst(OpID op, std::shared_ptr<Value> val, Type *ty, self_iterator *insert_before)
    : BaseInst<ZextInst>(ty, op, 1, insert_before), dest_ty_(ty) {
    set_operand(0, val);
}

Type *ZextInst::get_dest_type() const { return dest_ty_; }

std::string ZextInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(get_operand(0).get(), false);
    instr_ir += " to ";
    instr_ir += this->get_dest_type()->print();
    return instr_ir;
}

FpToSiInst::FpToSiInst(OpID op, std::shared_ptr<Value> val, Type *ty, BasicBlock *bb)
    : BaseInst<FpToSiInst>(ty, op, 1, bb), dest_ty_(ty) {
    set_operand(0, val);
}

Type *FpToSiInst::get_dest_type() const { return dest_ty_; }

std::string FpToSiInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(get_operand(0).get(), false);
    instr_ir += " to ";
    instr_ir += this->get_dest_type()->print();
    return instr_ir;
}

SiToFpInst::SiToFpInst(OpID op, std::shared_ptr<Value> val, Type *ty, BasicBlock *bb)
    : BaseInst<SiToFpInst>(ty, op, 1, bb), dest_ty_(ty) {
    set_operand(0, val);
}

Type *SiToFpInst::get_dest_type() const { return dest_ty_; }

std::string SiToFpInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(get_operand(0).get(), false);
    instr_ir += " to ";
    instr_ir += this->get_dest_type()->print();
    return instr_ir;
}

SwitchInst::SwitchInst(OpID op,
                       std::shared_ptr<Value> val,
                       std::shared_ptr<BasicBlock> default_block,
                       std::vector<std::shared_ptr<Value>> vals,
                       std::vector<std::shared_ptr<BasicBlock>> val_bbs,
                       Type *ty,
                       BasicBlock *bb)
    : BaseInst<SwitchInst>(ty, op, 2 + 2 * vals.size(), bb), test_val(val), default_block(default_block) {
    set_operand(0, val);
    set_operand(1, std::weak_ptr<Value>(default_block));
    for (size_t i = 0; i < vals.size(); i++) {
        set_operand(2 * i + 2, vals[i]);
        set_operand(2 * i + 3, std::weak_ptr<Value>(val_bbs[i]));
    }
}

SwitchInst::SwitchInst(OpID op,
                       std::shared_ptr<Value> val,
                       std::shared_ptr<BasicBlock> default_block,
                       std::vector<std::shared_ptr<Value>> vals,
                       std::vector<std::shared_ptr<BasicBlock>> val_bbs,
                       Type *ty,
                       Instruction::self_iterator *insert_before)
    : BaseInst<SwitchInst>(ty, op, 2 + 2 * vals.size(), insert_before), test_val(val), default_block(default_block) {
    set_operand(0, val);
    set_operand(1, std::weak_ptr<Value>(default_block));
    for (size_t i = 0; i < vals.size(); i++) {
        set_operand(2 * i + 2, vals[i]);
        set_operand(2 * i + 3, std::weak_ptr<Value>(val_bbs[i]));
    }
}

std::string SwitchInst::print() {
    std::string ir;
    ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    ir += " ";
    ir += print_as_op(get_operand(0).get(), true);
    ir += ", ";
    ir += print_as_op(get_operand(1).get(), true);
    ir += " [ ";

    for (size_t i = 2; i < this->get_num_operand(); i += 2) {
        ir += print_as_op(get_operand(i).get(), true);
        ir += ", ";
        ir += print_as_op(get_operand(i + 1).get(), true);
        ir += "\n";
    }
    ir += "]";
    return ir;
}

PhiInst::PhiInst(OpID op,
                 std::vector<std::shared_ptr<Value>> vals,
                 std::vector<std::shared_ptr<BasicBlock>> val_bbs,
                 Type *ty,
                 BasicBlock *bb)
    : BaseInst<PhiInst>(ty, op, 2 * vals.size(), bb) {
    for (size_t i = 0; i < vals.size(); i++) {
        set_operand(2 * i, vals[i]);
        set_operand(2 * i + 1, std::weak_ptr<Value>(val_bbs[i]));
    }
    // this->set_parent(bb);
}

PhiInst::PhiInst(OpID op,
                 std::vector<std::shared_ptr<Value>> vals,
                 std::vector<std::shared_ptr<BasicBlock>> val_bbs,
                 Type *ty,
                 Instruction::self_iterator *insert_before)
    : BaseInst<PhiInst>(ty, op, 2 * vals.size(), insert_before) {
    for (size_t i = 0; i < vals.size(); i++) {
        set_operand(2 * i, vals[i]);
        set_operand(2 * i + 1, std::weak_ptr<Value>(val_bbs[i]));
    }
    // this->set_parent(bb);
}

std::string PhiInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    if (get_num_operand() == 0) {
        instr_ir += "undef(zero operands)";
        return instr_ir;
    }
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    for (int i = 0; i < this->get_num_operand() / 2; i++) {
        if (i > 0)
            instr_ir += ", ";
        instr_ir += "[ ";
        instr_ir += print_as_op(get_operand(2 * i).get(), false);
        instr_ir += ", ";
        instr_ir += print_as_op(get_operand(2 * i + 1).get(), false);
        instr_ir += " ]";
    }
    if (this->get_num_operand() / 2 < this->get_parent()->get_pre_basic_blocks().size()) {
        for (auto pre_bb : this->get_parent()->get_pre_basic_blocks()) {
            if (std::find_if(get_operands().begin(), get_operands().end(), [pre_bb](const auto &op) {
                    return op.get() == pre_bb;
                }) == this->get_operands().end()) {
                // find a pre_bb is not in phi
                instr_ir += ", [ undef, " + print_as_op(pre_bb, false) + " ]";
            }
        }
    }
    return instr_ir;
}
