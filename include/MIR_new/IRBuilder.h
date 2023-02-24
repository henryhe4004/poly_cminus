#ifndef SYSYC_IRBUILDER_H
#define SYSYC_IRBUILDER_H

#include "BasicBlock.h"
#include "Instruction.h"
#include "Value.h"

class IRBuilder {
  private:
    std::shared_ptr<BasicBlock> BB_;
    Module *m_;

  public:
    IRBuilder(BasicBlock *bb, Module *m) : BB_(bb), m_(m){};
    ~IRBuilder() = default;
    Module *get_module() { return m_; }
    std::shared_ptr<BasicBlock> get_insert_block() { return this->BB_; }
    void set_insert_point(std::shared_ptr<BasicBlock> bb) { this->BB_ = bb; }  //IRbuilder的BB修改成bb
    std::shared_ptr<BinaryInst> create_iadd(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_add(lhs, rhs, this->BB_.get());
    }  //创建加法指令（以及其他算术指令）
    std::shared_ptr<BinaryInst> create_isub(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_sub(lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<BinaryInst> create_imul(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_mul(lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<BinaryInst> create_isdiv(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_sdiv(lhs, rhs, this->BB_.get());
    }

    std::shared_ptr<BinaryInst> create_srem(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_srem(lhs, rhs, this->BB_.get());
    }

    std::shared_ptr<BinaryInst> create_shl(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_shl(lhs, rhs, this->BB_.get());
    }

    std::shared_ptr<BinaryInst> create_ashr(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_ashr(lhs, rhs, this->BB_.get());
    }

    std::shared_ptr<BinaryInst> create_and_(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_and_(lhs, rhs, this->BB_.get());
    }

    std::shared_ptr<BinaryInst> create_lshr(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_lshr(lhs, rhs, this->BB_.get());
    }

    std::shared_ptr<CmpInst> create_icmp_eq(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return CmpInst::create_cmp(CmpOp::EQ, lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<CmpInst> create_icmp_ne(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return CmpInst::create_cmp(CmpOp::NE, lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<CmpInst> create_icmp_gt(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return CmpInst::create_cmp(CmpOp::GT, lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<CmpInst> create_icmp_ge(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return CmpInst::create_cmp(CmpOp::GE, lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<CmpInst> create_icmp_lt(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return CmpInst::create_cmp(CmpOp::LT, lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<CmpInst> create_icmp_le(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return CmpInst::create_cmp(CmpOp::LE, lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<SelectInst> create_select(std::shared_ptr<Value> test_bit,
                                              std::shared_ptr<Value> lhs,
                                              std::shared_ptr<Value> rhs) {
        return SelectInst::create_select(test_bit, lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<CallInst> create_call(std::shared_ptr<Function> func, std::vector<std::shared_ptr<Value>> args) {
        // assert(dynamic_cast<Function *>(func) && "func must be Function * type");
        return CallInst::create(func, args, this->BB_.get());
    }

    std::shared_ptr<BranchInst> create_br(std::shared_ptr<BasicBlock> if_true) {
        return BranchInst::create_br(if_true, this->BB_.get());
    }
    std::shared_ptr<BranchInst> create_cond_br(std::shared_ptr<Value> cond,
                                               std::shared_ptr<BasicBlock> if_true,
                                               std::shared_ptr<BasicBlock> if_false) {
        return BranchInst::create_cond_br(cond, if_true, if_false, this->BB_.get());
    }

    std::shared_ptr<ReturnInst> create_ret(std::shared_ptr<Value> val) {
        return ReturnInst::create_ret(val, this->BB_.get());
    }
    std::shared_ptr<ReturnInst> create_void_ret() { return ReturnInst::create_void_ret(this->BB_.get()); }

    std::shared_ptr<GetElementPtrInst> create_gep(std::shared_ptr<Value> ptr,
                                                  std::vector<std::shared_ptr<Value>> idxs) {
        return GetElementPtrInst::create_gep(ptr, idxs, this->BB_.get());
    }

    std::shared_ptr<StoreInst> create_store(std::shared_ptr<Value> val, std::shared_ptr<Value> ptr) {
        return StoreInst::create_store(val, ptr, this->BB_.get());
    }
    std::shared_ptr<LoadInst> create_load(Type *ty, std::shared_ptr<Value> ptr) {
        return LoadInst::create_load(ty, ptr, this->BB_.get());
    }
    std::shared_ptr<LoadInst> create_load(std::shared_ptr<Value> ptr) {
        assert(ptr->get_type()->is_pointer_type() && "ptr must be pointer type");
        return LoadInst::create_load(ptr->get_type()->get_pointer_element_type(), ptr, this->BB_.get());
    }

    std::shared_ptr<AllocaInst> create_alloca(Type *ty) { return AllocaInst::create_alloca(ty, this->BB_.get()); }
    std::shared_ptr<ZextInst> create_zext(std::shared_ptr<Value> val, Type *ty) {
        return ZextInst::create_zext(val, ty, this->BB_.get());
    }

    std::shared_ptr<SiToFpInst> create_sitofp(std::shared_ptr<Value> val, Type *ty) {
        return SiToFpInst::create_sitofp(val, ty, this->BB_.get());
    }
    std::shared_ptr<FpToSiInst> create_fptosi(std::shared_ptr<Value> val, Type *ty) {
        return FpToSiInst::create_fptosi(val, ty, this->BB_.get());
    }

    std::shared_ptr<FCmpInst> create_fcmp_ne(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return FCmpInst::create_fcmp(CmpOp::NE, lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<FCmpInst> create_fcmp_lt(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return FCmpInst::create_fcmp(CmpOp::LT, lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<FCmpInst> create_fcmp_le(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return FCmpInst::create_fcmp(CmpOp::LE, lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<FCmpInst> create_fcmp_ge(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return FCmpInst::create_fcmp(CmpOp::GE, lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<FCmpInst> create_fcmp_gt(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return FCmpInst::create_fcmp(CmpOp::GT, lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<FCmpInst> create_fcmp_eq(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return FCmpInst::create_fcmp(CmpOp::EQ, lhs, rhs, this->BB_.get());
    }

    std::shared_ptr<BinaryInst> create_fadd(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_fadd(lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<BinaryInst> create_fsub(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_fsub(lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<BinaryInst> create_fmul(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_fmul(lhs, rhs, this->BB_.get());
    }
    std::shared_ptr<BinaryInst> create_fdiv(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
        return BinaryInst::create_fdiv(lhs, rhs, this->BB_.get());
    }
};

#endif  // SYSYC_IRBUILDER_H
