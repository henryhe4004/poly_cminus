#pragma once
#include "IRprinter.h"
#include "Instruction.h"
class MovInst : public BaseInst<MovInst> {
    friend BaseInst<MovInst>;

  protected:
    MovInst(std::shared_ptr<Value> to, std::shared_ptr<Value> from, BasicBlock *bb)
        : BaseInst<MovInst>(Type::get_void_type(bb->get_module()), Instruction::mov, 2, bb) {
        set_operand(0, to);
        set_operand(1, from);
    }

  public:
    // physical register?
    static std::shared_ptr<MovInst> create_mov(std::shared_ptr<Value> to, std::shared_ptr<Value> from, BasicBlock *bb) {
        return create(to, from, bb);
    }
    auto get_from() { return get_operand(1); }
    auto get_to() { return get_operand(0); }
    virtual std::string print() override;
};

class MuladdInst : public BaseInst<MuladdInst> {
    friend BaseInst<MuladdInst>;

  private:
    MuladdInst(Type *ty,
               std::shared_ptr<Value> add_op,
               std::shared_ptr<Value> mul_op1,
               std::shared_ptr<Value> mul_op2,
               BasicBlock *bb)
        : BaseInst<MuladdInst>(ty, OpID::muladd, 3, bb) {
        set_operand(0, add_op);
        set_operand(1, mul_op1);
        set_operand(2, mul_op2);
    }
    MuladdInst(Type *ty,
               std::shared_ptr<Value> add_op,
               std::shared_ptr<Value> mul_op1,
               std::shared_ptr<Value> mul_op2,
               self_iterator *it = nullptr)
        : BaseInst<MuladdInst>(ty, OpID::muladd, 3, it) {
        set_operand(0, add_op);
        set_operand(1, mul_op1);
        set_operand(2, mul_op2);
    }

  public:
    static std::shared_ptr<MuladdInst> create_muladd(std::shared_ptr<Value> add_op,
                                                     std::shared_ptr<Value> mul_op1,
                                                     std::shared_ptr<Value> mul_op2,
                                                     BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), add_op, mul_op1, mul_op2, bb);
    }
    static std::shared_ptr<MuladdInst> create_muladd(std::shared_ptr<Value> add_op,
                                                     std::shared_ptr<Value> mul_op1,
                                                     std::shared_ptr<Value> mul_op2,
                                                     self_iterator *insert_before) {
        return create(Type::get_int32_type((**insert_before)->get_parent()->get_module()),
                      add_op,
                      mul_op1,
                      mul_op2,
                      insert_before);
    }
    virtual std::string print() override;
};

class MulsubInst : public BaseInst<MulsubInst> {
    friend BaseInst<MulsubInst>;

  private:
    MulsubInst(Type *ty,
               std::shared_ptr<Value> sub_op,
               std::shared_ptr<Value> mul_op1,
               std::shared_ptr<Value> mul_op2,
               BasicBlock *bb)
        : BaseInst<MulsubInst>(ty, OpID::mulsub, 3, bb) {
        set_operand(0, sub_op);
        set_operand(1, mul_op1);
        set_operand(2, mul_op2);
    }
    MulsubInst(Type *ty,
               std::shared_ptr<Value> sub_op,
               std::shared_ptr<Value> mul_op1,
               std::shared_ptr<Value> mul_op2,
               self_iterator *it = nullptr)
        : BaseInst<MulsubInst>(ty, OpID::mulsub, 3, it) {
        set_operand(0, sub_op);
        set_operand(1, mul_op1);
        set_operand(2, mul_op2);
    }

  public:
    static std::shared_ptr<MulsubInst> create_mulsub(std::shared_ptr<Value> sub_op,
                                                     std::shared_ptr<Value> mul_op1,
                                                     std::shared_ptr<Value> mul_op2,
                                                     BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), sub_op, mul_op1, mul_op2, bb);
    }
    static std::shared_ptr<MulsubInst> create_mulsub(std::shared_ptr<Value> sub_op,
                                                     std::shared_ptr<Value> mul_op1,
                                                     std::shared_ptr<Value> mul_op2,
                                                     self_iterator *insert_before) {
        return create(Type::get_int32_type((**insert_before)->get_parent()->get_module()),
                      sub_op,
                      mul_op1,
                      mul_op2,
                      insert_before);
    }
    virtual std::string print() override;
};

// class RsubInst : public BaseInst<RsubInst> {
//     friend BaseInst<RsubInst>;

//   private:
//     RsubInst(std::shared_ptr<Value> op1, std::shared_ptr<Value> op2, BasicBlock *bb)
//         : BaseInst<RsubInst>(op1->get_type(), OpID::rsub, 2, bb) {
//         set_operand(0, op1);
//         set_operand(1, op2);
//     }
//     RsubInst(std::shared_ptr<Value> op1, std::shared_ptr<Value> op2, self_iterator *it)
//         : BaseInst<RsubInst>(op1->get_type(), OpID::rsub, 2, it) {
//         set_operand(0, op1);
//         set_operand(1, op2);
//     }

//   public:
//     static create_rsub(std::shared_ptr<Value> op1, std::shared_ptr<Value> op2, self_iterator *it) {
//         return create(op1, op2, it);
//     }
// }