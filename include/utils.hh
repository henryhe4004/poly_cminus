#ifndef __UTILS_HH__
#define __UTILS_HH__
#include "BasicBlock.h"
#include "Constant.h"
#include "GlobalVariable.h"
#include "Instruction.h"
#include "Value.h"
#include "config.hh"
#include "errorcode.hh"

#include <map>
#include <memory>
#include <optional>

// c/p from https://llvm.org/doxygen/PatternMatch_8h_source.html
template <typename Class>
struct bind_ty {
    Class *&VR;

    bind_ty(Class *&V) : VR(V) {}

    template <typename ITy>
    bool match(ITy *V) {
        if (auto *CV = dynamic_cast<Class *>(V)) {
            VR = CV;
            return true;
        }
        return false;
    }
};

/// Match a value, capturing it if we match.
inline bind_ty<Value> m_value(Value *&V) { return V; }
inline bind_ty<BasicBlock> m_bb(BasicBlock *&V) { return V; }
inline bind_ty<AllocaInst> m_alloca(AllocaInst *&V) { return V; }
inline bind_ty<CallInst> m_call(CallInst *&V) { return V; }
inline bind_ty<ReturnInst> m_ret(ReturnInst *&V) { return V; }
inline bind_ty<PhiInst> m_phi(PhiInst *&V) { return V; }
inline bind_ty<BranchInst> m_br(BranchInst *&V) { return V; }
/// Match a Constant, capturing the value if we match.
inline bind_ty<Constant> m_constant(Constant *&C) { return C; }

/// Match a ConstantInt, capturing the value if we match.
inline bind_ty<ConstantInt> m_constantInt(ConstantInt *&CI) { return CI; }

/// Match a ConstantFP, capturing the value if we match.
inline bind_ty<ConstantFP> m_constantFP(ConstantFP *&C) { return C; }
template <typename Op_t, unsigned Opcode>
struct CastClass_match {
    Op_t Op;

    CastClass_match(const Op_t &OpMatch) : Op(OpMatch) {}

    template <typename OpTy>
    bool match(OpTy *V) {
        if (auto *O = dynamic_cast<Instruction *>(V))
            return O->get_instr_type() == Opcode && O->get_num_operand() >= 1 && Op.match(O->get_operand(0).get());
        return false;
    }
};

template <typename OpTy>
inline CastClass_match<OpTy, Instruction::inttoptr> m_inttoptr(const OpTy &Op) {
    return CastClass_match<OpTy, Instruction::inttoptr>(Op);
}

template <typename OpTy>
inline CastClass_match<OpTy, Instruction::ptrtoint> m_ptrtoint(const OpTy &Op) {
    return CastClass_match<OpTy, Instruction::ptrtoint>(Op);
}

template <typename OpTy>
inline CastClass_match<OpTy, Instruction::ret> m_ret(const OpTy &Op) {
    return CastClass_match<OpTy, Instruction::ret>(Op);
}

template <typename OpTy>
inline CastClass_match<OpTy, Instruction::br> m_br(const OpTy &Op) {
    return CastClass_match<OpTy, Instruction::br>(Op);
}

template <typename OpTy>
inline CastClass_match<OpTy, Instruction::zext> m_zext(const OpTy &Op) {
    return CastClass_match<OpTy, Instruction::zext>(Op);
}

template <typename Val, typename Pattern>
bool match(Val *V, const Pattern &P) {
    return const_cast<Pattern &>(P).match(V);
}

template <typename LHS_t, typename RHS_t, unsigned Opcode, bool Commutable = false>
struct BinaryOp_match {
    LHS_t L;
    RHS_t R;

    // The evaluation order is always stable, regardless of Commutability.
    // The LHS is always matched first.
    BinaryOp_match(const LHS_t &LHS, const RHS_t &RHS) : L(LHS), R(RHS) {}

    template <typename OpTy>
    inline bool match(unsigned Opc, OpTy *V) {
        auto *I = dynamic_cast<BinaryInst *>(V);
        if (I and I->get_instr_type() == Opc) {
            return (L.match(I->get_operand(0).get()) && R.match(I->get_operand(1).get())) ||
                   (Commutable && L.match(I->get_operand(1).get()) && R.match(I->get_operand(0).get()));
        }
        return false;
    }

    template <typename OpTy>
    bool match(OpTy *V) {
        return match(Opcode, V);
    }
};
template <typename LHS, typename RHS>
inline BinaryOp_match<LHS, RHS, Instruction::add, true> m_Add(const LHS &L, const RHS &R) {
    return BinaryOp_match<LHS, RHS, Instruction::add, true>(L, R);
}

template <typename LHS, typename RHS>
inline BinaryOp_match<LHS, RHS, Instruction::mul, true> m_Mul(const LHS &L, const RHS &R) {
    return BinaryOp_match<LHS, RHS, Instruction::mul, true>(L, R);
}

template <typename LHS, typename RHS>
inline BinaryOp_match<LHS, RHS, Instruction::sub> m_Sub(const LHS &L, const RHS &R) {
    return BinaryOp_match<LHS, RHS, Instruction::sub>(L, R);
}

template <typename LHS, typename RHS>
inline BinaryOp_match<LHS, RHS, Instruction::shl> m_Shl(const LHS &L, const RHS &R) {
    return BinaryOp_match<LHS, RHS, Instruction::shl>(L, R);
}

template <typename LHS, typename RHS>
inline BinaryOp_match<LHS, RHS, Instruction::ashr> m_Ashr(const LHS &L, const RHS &R) {
    return BinaryOp_match<LHS, RHS, Instruction::ashr>(L, R);
}

template <typename LHS, typename RHS>
inline BinaryOp_match<LHS, RHS, Instruction::lshr> m_Lshr(const LHS &L, const RHS &R) {
    return BinaryOp_match<LHS, RHS, Instruction::lshr>(L, R);
}

/// Match a specified Value*.
struct specificval_ty {
    const Value *Val;

    specificval_ty(const Value *V) : Val(V) {}

    template <typename ITy>
    bool match(ITy *V) {
        return V == Val;
    }
};
template <typename T>
bool isa(Value *);

/// Match if we have a specific specified value.
inline specificval_ty m_specific(const Value *V) { return V; }
namespace utils {
inline bool is_const(Value *v) {
    if (dynamic_cast<Constant *>(v))
        return true;
    return false;
}

inline bool is_const_int(Value *v) {
    if (dynamic_cast<ConstantInt *>(v))
        return true;
    return false;
}

inline bool is_const_int(Value *v, int n) {
    if (isa<ConstantInt>(v) and dynamic_cast<ConstantInt *>(v)->get_value() == n)
        return true;
    return false;
}

inline bool is_const_fp(Value *v) {
    if (dynamic_cast<ConstantFP *>(v))
        return true;
    return false;
}

inline bool is_global_var(Value *v) {
    if (dynamic_cast<GlobalVariable *>(v))
        return true;
    return false;
}

inline bool is_instruction(Value *v) {
    if (dynamic_cast<Instruction *>(v))
        return true;
    return false;
}

inline std::optional<int> get_const_int_val(Value *v) {
    if (auto cv = dynamic_cast<ConstantInt *>(v))
        return cv->get_value();
    return std::nullopt;
}

inline std::optional<float> get_const_float_val(Value *v) {
    if (auto cv = dynamic_cast<ConstantFP *>(v))
        return cv->get_value();
    return std::nullopt;
}

std::shared_ptr<Value> to_i32(std::shared_ptr<Value> v, Module *m);

// 如果v1、v2都是常数，返回两个常数相运算的常数结果，否则返回相应的指令
std::shared_ptr<Value> add_i32(std::shared_ptr<Value> v1, std::shared_ptr<Value> v2, Module *m);

std::shared_ptr<Value> sub_i32(std::shared_ptr<Value> v1, std::shared_ptr<Value> v2, Module *m);

std::shared_ptr<Value> mul_i32(std::shared_ptr<Value> v1, std::shared_ptr<Value> v2, Module *m);

std::shared_ptr<Value> add_i32(std::shared_ptr<Value> v1,
                               std::shared_ptr<Value> v2,
                               Module *m,
                               Instruction::self_iterator *insert_point);

std::shared_ptr<Value> sub_i32(std::shared_ptr<Value> v1,
                               std::shared_ptr<Value> v2,
                               Module *m,
                               Instruction::self_iterator *insert_point);

std::shared_ptr<Value> mul_i32(std::shared_ptr<Value> v1,
                               std::shared_ptr<Value> v2,
                               Module *m,
                               Instruction::self_iterator *insert_point);
std::shared_ptr<Value> sdiv_i32(std::shared_ptr<Value> v1,
                                std::shared_ptr<Value> v2,
                                Module *m,
                                Instruction::self_iterator *insert_point);

std::vector<Instruction *> clone_instructions(std::vector<Instruction *> all_inst,
                                              BasicBlock *bb,
                                              std::map<Value *, Value *> &to_new_operand);
void replace_phi(std::vector<std::shared_ptr<BasicBlock>> new_blocks, std::map<Value *, Value *> &to_new_operand);
void clone_blocks(std::vector<BasicBlock *> blocks,
                  std::vector<std::shared_ptr<BasicBlock>> new_blocks,
                  std::map<Value *, Value *> &to_new_operand);
void replace_phi_indexbb(std::map<Instruction *, std::map<BasicBlock *, Value *>> &phi_index_bb_map);
std::shared_ptr<BasicBlock> init_create_block(BasicBlock *block, std::string suffix);
}  // namespace utils

template <typename T>
bool isa(std::shared_ptr<Value> val) {
    return std::dynamic_pointer_cast<T>(val) != nullptr;
}
template <typename T>
bool isa(Value *val) {
    return dynamic_cast<T *>(val) != nullptr;
}
template <typename First, typename Second, typename... Rest>
inline bool isa(Value *val) {
    return isa<First>(val) || isa<Second, Rest...>(val);
}

template <typename First, typename Second, typename... Rest>
inline bool isa(std::shared_ptr<Value> val) {
    return isa<First>(val) || isa<Second, Rest...>(val);
}
#endif
