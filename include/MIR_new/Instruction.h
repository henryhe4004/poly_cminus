#ifndef SYSYC_INSTRUCTION_H
#define SYSYC_INSTRUCTION_H

#include "BasicBlock.h"
#include "Constant.h"
#include "Type.h"
#include "User.h"
#include "enum.h"
#include "errorcode.hh"
#include "logging.hpp"

#include <initializer_list>

class BasicBlock;
class Function;

class Instruction : public User {
  public:
    using self_iterator = inst_list_iterator_type;
    enum OpID {
        // Terminator Instructions
        ret,
        br,
        switch1,
        // Standard binary operators
        add,
        sub,
        mul,
        sdiv,
        srem,
        // bit operators
        and1,
        or1,
        xor1,
        shl,   // 左移
        lshr,  // 逻辑右移，高位填0
        ashr,  // 算数右移，见 https://llvm.org/docs/LangRef.html#ashr-instruction
        and_,  // and
        // float binary operators
        fadd,
        fsub,
        fmul,
        fdiv,
        // Memory operators
        alloca,
        load,
        store,
        // Other operators
        cmp,
        fcmp,
        select,
        phi,
        call,
        getelementptr,
        zext,  // zero extend
        fptosi,
        sitofp,
        ptrtoint,
        inttoptr,
        // LIR
        mov,
        muladd,
        mulsub,
        rsub  // 反向减法
    };
    // create instruction, auto insert to bb
    // ty here is result type
    Instruction(Type *ty, OpID id, unsigned num_ops, BasicBlock *parent);
    Instruction(Type *ty, OpID id, unsigned num_ops, self_iterator * = nullptr);
    inline const BasicBlock *get_parent() const { return parent_; }
    inline BasicBlock *get_parent() { return parent_; }
    inline const std::shared_ptr<BasicBlock> get_parent_shared() const {
        return std::dynamic_pointer_cast<BasicBlock>(parent_->shared_from_this());
    }
    inline std::shared_ptr<BasicBlock> get_parent_shared() {
        return std::dynamic_pointer_cast<BasicBlock>(parent_->shared_from_this());
    }
    auto get_iterator() { return parent_->find_instr(get_shared_ptr()); }
    void erase_from_parent() {
        exit_if(get_parent() == nullptr, MANIPULATE_INST_WITHOUT_PARENT);
        remove_use_of_ops();
        get_parent()->delete_instr(get_shared_ptr());
    }

    template <typename T, typename = std::enable_if_t<std::is_base_of<Instruction, T>::value>>
    void replace_with(std::shared_ptr<T> inst) {
        exit_if(get_parent() == nullptr, MANIPULATE_INST_WITHOUT_PARENT);
        inst->set_parent(get_parent());
        get_parent()->insert_before(get_shared_ptr(), inst);
        this->replace_all_use_with(inst);
    }

    template <typename T, typename... Args, typename = std::enable_if_t<std::is_base_of<Instruction, T>::value>>
    void replace_with(std::shared_ptr<T> inst, std::shared_ptr<Args>... rest) {
        get_parent()->insert_before(get_shared_ptr(), inst);
        replace_with(rest...);
    }

    void replace_with(std::initializer_list<std::shared_ptr<Instruction>> insts) {
        exit_if(get_parent() == nullptr, MANIPULATE_INST_WITHOUT_PARENT);
        auto sptr = get_shared_ptr();
        for (auto &ptr : insts)
            get_parent()->insert_before(sptr, ptr);

        auto it = insts.end();
        it--;
        this->replace_all_use_with(*it);
        this->remove_use_of_ops();
    }

    std::shared_ptr<Instruction> get_shared_ptr() {
        return std::dynamic_pointer_cast<Instruction>(this->shared_from_this());
    }

    void set_parent(BasicBlock *parent) { this->parent_ = parent; }
    /// \return the function this instruction belongs to.
    Function *get_function();
    Module *get_module();

    OpID get_instr_type() const { return op_id_; }
    std::string get_instr_op_name() {
        switch (op_id_) {
            case ret:
                return "ret";
                break;
            case br:
                return "br";
                break;
            case switch1:
                return "switch";
                break;
            case add:
                return "add";
                break;
            case sub:
                return "sub";
                break;
            case mul:
                return "mul";
                break;
            case sdiv:
                return "sdiv";
                break;
            case srem:
                return "srem";
                break;
            case shl:
                return "shl";
                break;
            case ashr:
                return "ashr";
                break;
            case and_:
                return "and";
                break;
            case lshr:
                return "lshr";
                break;
            case fadd:
                return "fadd";
                break;
            case fsub:
                return "fsub";
                break;
            case fmul:
                return "fmul";
                break;
            case fdiv:
                return "fdiv";
                break;
            case alloca:
                return "alloca";
                break;
            case load:
                return "load";
                break;
            case store:
                return "store";
                break;
            case cmp:
                return "cmp";
                break;
            case fcmp:
                return "fcmp";
                break;
            case phi:
                return "phi";
                break;
            case call:
                return "call";
                break;
            case getelementptr:
                return "getelementptr";
                break;
            case zext:
                return "zext";
                break;
            case fptosi:
                return "fptosi";
                break;
            case sitofp:
                return "sitofp";
                break;
            case mov:
                return "mov";
                break;
            case ptrtoint:
                return "ptrtoint";
                break;
            case inttoptr:
                return "inttoptr";
                break;
            case muladd:
                return "muladd";
                break;
            case mulsub:
                return "mulsub";
                break;
            case rsub:
                return "rsub";
                break;
            case and1:
                return "and";
                break;
            case or1:
                return "or";
                break;
            case xor1:
                return "xor";
                break;
            case select:
                return "select";
                break;
            default:
                exit(200);
                LOG_ERROR << "unable to find opname";
                return "";
                break;
        }
    }

    bool is_void() {
        return ((op_id_ == ret) || (op_id_ == br) || (op_id_ == switch1) || (op_id_ == store) || (op_id_ == mov) ||
                (op_id_ == call && this->get_type()->is_void_type()));
    }
    bool is_phi() { return op_id_ == phi; }
    bool is_store() { return op_id_ == store; }
    bool is_alloca() { return op_id_ == alloca; }
    bool is_ret() { return op_id_ == ret; }
    bool is_load() { return op_id_ == load; }
    bool is_br() { return op_id_ == br; }
    bool is_switch() { return op_id_ == switch1; }

    bool is_add() { return op_id_ == add; }
    bool is_sub() { return op_id_ == sub; }
    bool is_mul() { return op_id_ == mul; }
    bool is_div() { return op_id_ == sdiv; }
    bool is_srem() { return op_id_ == srem; }
    bool is_shl() { return op_id_ == shl; }
    bool is_ashr() { return op_id_ == ashr; }
    bool is_and_() { return op_id_ == and_; }
    bool is_lshr() { return op_id_ == lshr; }
    bool is_and() { return op_id_ == and1; }
    bool is_or() { return op_id_ == or1; }
    bool is_xor() { return op_id_ == xor1; }

    bool is_fadd() { return op_id_ == fadd; }
    bool is_fsub() { return op_id_ == fsub; }
    bool is_fmul() { return op_id_ == fmul; }
    bool is_fdiv() { return op_id_ == fdiv; }
    bool is_fp2si() { return op_id_ == fptosi; }
    bool is_si2fp() { return op_id_ == sitofp; }

    bool is_cmp() { return op_id_ == cmp; }
    bool is_fcmp() { return op_id_ == fcmp; }

    bool is_call() { return op_id_ == call; }
    bool is_gep() { return op_id_ == getelementptr; }
    bool is_zext() { return op_id_ == zext; }

    bool isBinary() {
        return (is_add() || is_sub() || is_mul() || is_div() || is_srem() || is_shl() || is_ashr() || is_lshr() ||
                is_and() || is_and_() || is_or() || is_xor() || is_fadd() || is_fsub() || is_fmul() || is_fdiv()) &&
               (get_num_operand() == 2);
    }
    bool is_mov() { return op_id_ == mov; }
    bool is_ptrtoint() { return op_id_ == ptrtoint; }
    bool is_inttoptr() { return op_id_ == inttoptr; }
    bool is_muladd() { return op_id_ == muladd; }
    bool is_mulsub() { return op_id_ == mulsub; }
    bool is_rsub() { return op_id_ == rsub; }
    bool isTerminator() { return is_br() || is_ret() || is_switch(); }
    bool is_select() {return op_id_ == select;}

    // help functions for common expression pass
    virtual bool is_hash_able() const { return false; }
    virtual std::size_t get_hash() const {
        LOG_ERROR << "Inst: hash method not implemented in subclass yet!";
        if (this->is_hash_able())
            LOG_ERROR << "Inst: wrong is_hash_able function";
        exit(ERROR_IN_LCE);
        return 0;
    }
    std::size_t get_base_hash() const {
        std::size_t ret = 0;
        for (int i = 0; i < this->get_num_operand(); ++i) {
            auto op = this->get_operand(i);
            auto ptrval = reinterpret_cast<uintptr_t>(op.get());
            ret = ret * mod + ptrval;
        }
        ret = ret * mod + static_cast<int>(this->get_instr_type());
        ret = ret * mod + reinterpret_cast<uintptr_t>(this->get_type());
        return ret;
    }
    constexpr static std::size_t mod = 998244353;
    virtual bool equal(const std::shared_ptr<Instruction> &rhs) const {
        if (this->get_instr_type() != rhs->get_instr_type())
            return false;
        if (this->get_type() != rhs->get_type())
            return false;
        if (this->get_num_operand() != rhs->get_num_operand())
            return false;
        for (int i = 0; i < get_num_operand(); ++i)
            if (this->get_operand(i).get() != rhs->get_operand(i).get())
                return false;
        return true;
    }

  private:
    OpID op_id_;
    unsigned num_ops_;
    BasicBlock *parent_;
};

template <typename T>
struct tag {
    using type = T;
};

template <typename... Ts>
struct select_last {
    // Use a fold-expression to fold the comma operator over the parameter pack.
    using type = typename decltype((tag<Ts>{}, ...))::type;
};

template <typename... T, std::size_t... I, typename U>
auto retype_last_element(const std::tuple<T...> &t, std::index_sequence<I...>, U ptr) {
    return std::make_tuple(std::get<I>(t)..., ptr);
}

// TODO: 修改最后一个参数为 Instruction*类型，使用 get_iterator 获取迭代器
/// \tparam Inst the derived instruction class to be added with the static factory method
/// \brief 三个重载，现在create传入的bb应该为非空，当最后一个参数为nullptr时，这个指针会被认为是 self_iterator*类型
template <typename Inst>
struct BaseInst : public Instruction {
    template <typename... T, std::size_t... I>
    static Inst *new_impl(const std::tuple<T...> &t, std::index_sequence<I...>) {
        return new Inst(std::get<I>(t)...);
    }
    /// \tparam Args enabled when last element of Args is the same as `BasicBlock*`
    template <typename... Args>
    static std::enable_if_t<std::is_same_v<typename select_last<Args...>::type, BasicBlock *>, std::shared_ptr<Inst>>
    create(Args... args) {
        auto ptr = std::shared_ptr<Inst>(new Inst(args...));
        if (ptr->get_parent())
            ptr->get_parent()->add_instruction(ptr);
        return ptr;
    }
    /// \tparam Args enabled when last element of Args is convertible to an iterator pointer
    template <typename... Args>
    static std::enable_if_t<std::is_convertible_v<typename select_last<Args...>::type, Instruction::self_iterator *>,
                            std::shared_ptr<Inst>>
    create(Args... args) {
        Instruction::self_iterator *insert_before =
            std::get<sizeof...(Args) - 1>(std::forward_as_tuple(std::forward<Args>(args)...));
        auto args_as_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
        auto index_seq = std::make_index_sequence<sizeof...(Args) - 1>{};
        auto index_seq_whole = std::make_index_sequence<sizeof...(Args)>{};
        auto sub_tuple = retype_last_element(args_as_tuple, index_seq, insert_before);
        auto i = std::shared_ptr<Inst>(new_impl(sub_tuple, index_seq_whole));
        if (insert_before) {
            (**insert_before)->get_parent()->get_instructions().insert(*insert_before, i);
            i->set_parent((**insert_before)->get_parent());
        }
        return i;
    }
    template <typename... Args>
    static std::enable_if_t<
        not std::is_convertible_v<typename select_last<Args...>::type, Instruction::self_iterator *> and
            not std::is_convertible_v<typename select_last<Args...>::type, BasicBlock *>,

        std::shared_ptr<Inst>>
    create(Args... args) {
        auto i = std::shared_ptr<Inst>(new Inst(args...));
        return i;
    }
    // pass c'tor arguments to Instruction
    template <typename... Args>
    BaseInst(Args... args) : Instruction(std::forward<Args>(args)...) {}
};

enum class shift_type_t { LSL, LSR, ASR };

class BinaryInst : public BaseInst<BinaryInst> {
    friend BaseInst<BinaryInst>;

  private:
    BinaryInst(Type *ty, OpID id, std::shared_ptr<Value> v1, std::shared_ptr<Value> v2, BasicBlock *bb);
    BinaryInst(Type *ty,
               OpID id,
               std::shared_ptr<Value> v1,
               std::shared_ptr<Value> v2,
               Instruction::self_iterator * = nullptr);

    BinaryInst(Type *ty, OpID id, BasicBlock *bb) : BaseInst<BinaryInst>(ty, id, 2, bb) {}

    BinaryInst(Type *ty, OpID id, Instruction::self_iterator *insert_before = nullptr)
        : BaseInst<BinaryInst>(ty, id, 2, insert_before) {}

  public:
    // create add instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_add(std::shared_ptr<Value> v1,
                                                  std::shared_ptr<Value> v2,
                                                  BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), Instruction::add, v1, v2, bb);
    }
    static std::shared_ptr<BinaryInst> create_add(std::shared_ptr<Value> v1,
                                                  std::shared_ptr<Value> v2,
                                                  Instruction::self_iterator * = nullptr);
    // create sub instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_sub(std::shared_ptr<Value> v1,
                                                  std::shared_ptr<Value> v2,
                                                  BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), Instruction::sub, v1, v2, bb);
    }

    static std::shared_ptr<BinaryInst> create_sub(std::shared_ptr<Value> v1,
                                                  std::shared_ptr<Value> v2,
                                                  Instruction::self_iterator *insert_before = nullptr) {
        return create(v1->get_type(), Instruction::sub, v1, v2, insert_before);
    }

    static std::shared_ptr<BinaryInst> create_rsub(std::shared_ptr<Value> v1,
                                                   std::shared_ptr<Value> v2,
                                                   Instruction::self_iterator *insert_before = nullptr) {
        return create(v1->get_type(), Instruction::rsub, v1, v2, insert_before);
    }

    // create mul instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_mul(std::shared_ptr<Value> v1,
                                                  std::shared_ptr<Value> v2,
                                                  BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), Instruction::mul, v1, v2, bb);
    }
    static std::shared_ptr<BinaryInst> create_mul(std::shared_ptr<Value> v1,
                                                  std::shared_ptr<Value> v2,
                                                  Instruction::self_iterator *);

    // create Div instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_sdiv(std::shared_ptr<Value> v1,
                                                   std::shared_ptr<Value> v2,
                                                   BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), Instruction::sdiv, v1, v2, bb);
    }

    // create Srem instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_srem(std::shared_ptr<Value> v1,
                                                   std::shared_ptr<Value> v2,
                                                   BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), Instruction::srem, v1, v2, bb);
    }

    // create shl instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_shl(std::shared_ptr<Value> v1,
                                                  std::shared_ptr<Value> v2,
                                                  BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), Instruction::shl, v1, v2, bb);
    }

    // create ashr instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_ashr(std::shared_ptr<Value> v1,
                                                   std::shared_ptr<Value> v2,
                                                   BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), Instruction::ashr, v1, v2, bb);
    }

    // create and_ instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_and_(std::shared_ptr<Value> v1,
                                                   std::shared_ptr<Value> v2,
                                                   BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), Instruction::and_, v1, v2, bb);
    }

    // create lshr instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_lshr(std::shared_ptr<Value> v1,
                                                   std::shared_ptr<Value> v2,
                                                   BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), Instruction::lshr, v1, v2, bb);
    }
    // create and instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_and(std::shared_ptr<Value> v1,
                                                  std::shared_ptr<Value> v2,
                                                  BasicBlock *bb) {
        return create(v1->get_type(), Instruction::and1, v1, v2, bb);
    }
    // create or instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_or(std::shared_ptr<Value> v1, std::shared_ptr<Value> v2, BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), Instruction::or1, v1, v2, bb);
    }
    // create xor instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_xor(std::shared_ptr<Value> v1,
                                                  std::shared_ptr<Value> v2,
                                                  BasicBlock *bb) {
        return create(Type::get_int32_type(bb->get_module()), Instruction::xor1, v1, v2, bb);
    }

    // create fadd instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_fadd(std::shared_ptr<Value> v1,
                                                   std::shared_ptr<Value> v2,
                                                   BasicBlock *bb) {
        return create(Type::get_float_type(bb->get_module()), Instruction::fadd, v1, v2, bb);
    }

    // create fsub instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_fsub(std::shared_ptr<Value> v1,
                                                   std::shared_ptr<Value> v2,
                                                   BasicBlock *bb) {
        return create(Type::get_float_type(bb->get_module()), Instruction::fsub, v1, v2, bb);
    }

    // create fmul instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_fmul(std::shared_ptr<Value> v1,
                                                   std::shared_ptr<Value> v2,
                                                   BasicBlock *bb) {
        return create(Type::get_float_type(bb->get_module()), Instruction::fmul, v1, v2, bb);
    }

    // create fDiv instruction, auto insert to bb
    static std::shared_ptr<BinaryInst> create_fdiv(std::shared_ptr<Value> v1,
                                                   std::shared_ptr<Value> v2,
                                                   BasicBlock *bb) {
        return create(Type::get_float_type(bb->get_module()), Instruction::fdiv, v1, v2, bb);
    }

    virtual std::string print() override;

    void set_flag() { update_flag = true; }
    void unset_flag() { update_flag = false; }
    bool set() const { return update_flag; }
    shift_type_t get_op2_shift_type() { return op2_sh_type; }
    int get_op2_shift_bits() { return op2_sh_num; }
    bool set_op2_shift(shift_type_t t, int n) {
        if (n == 0)
            op2_sh_type = shift_type_t::LSL;
        else
            op2_sh_type = t;
        if (op2_sh_type == shift_type_t::LSL && !(n >= 0 && n <= 31)) {
            LOG_ERROR << "illegal LSL number";
            return false;
        }
        if ((op2_sh_type == shift_type_t::LSR || op2_sh_type == shift_type_t::ASR) && !(n >= 1 && n <= 32)) {
            LOG_ERROR << "illegal (A/L)SR number";
            return false;
        }
        op2_sh_num = n;
        return true;
    }

    virtual std::size_t get_hash() const override {
        auto ret = this->get_base_hash();
        ret = ret * mod + this->op2_sh_num;
        ret = ret * mod + static_cast<int>(this->op2_sh_type);
        ret = ret * mod + update_flag;
        return ret;
    }
    virtual bool equal(const std::shared_ptr<Instruction> &rhs) const override {
        if (this->Instruction::equal(rhs) == false)
            return false;
        auto myinst = std::dynamic_pointer_cast<BinaryInst>(rhs);
        if (myinst == nullptr or op2_sh_num != myinst->get_op2_shift_bits() or
            op2_sh_type != myinst->get_op2_shift_type() or update_flag != myinst->set())
            return false;

        return true;
    }
    virtual bool is_hash_able() const override { return true; }

  private:
    void assertValid();
    int op2_sh_num = 0;
    bool update_flag = false;
    shift_type_t op2_sh_type = shift_type_t::LSL;
};
BETTER_ENUM(CmpOp, char, EQ, NE, GT, GE, LT, LE);

class CmpInst : public BaseInst<CmpInst> {
    friend BaseInst<CmpInst>;

  public:
    // enum CmpOp {
    //     EQ,  // ==
    //     NE,  // !=
    //     GT,  // >
    //     GE,  // >=
    //     LT,  // <
    //     LE   // <=
    // };

  private:
    CmpInst(Type *ty, CmpOp op, std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs, BasicBlock *bb);

  public:
    static std::shared_ptr<CmpInst> create_cmp(CmpOp op,
                                               std::shared_ptr<Value> lhs,
                                               std::shared_ptr<Value> rhs,
                                               BasicBlock *bb) {
        return create(Type::get_int1_type(bb->get_module()), op, lhs, rhs, bb);
    }
    // Module *m);

    CmpOp get_cmp_op() { return cmp_op_; }
    virtual std::size_t get_hash() const override {
        std::size_t ret = this->get_base_hash();
        ret = ret * mod + static_cast<int>(cmp_op_);
        return ret;
    }
    virtual bool equal(const std::shared_ptr<Instruction> &rhs) const override {
        if (this->Instruction::equal(rhs) == false)
            return false;
        auto cmpinst = std::dynamic_pointer_cast<CmpInst>(rhs);
        if (cmpinst == nullptr or cmp_op_ != cmpinst->cmp_op_)
            return false;
        return true;
    }
    virtual bool is_hash_able() const override { return true; }
    virtual std::string print() override;

  private:
    CmpOp cmp_op_;

    void assertValid();
};

class FCmpInst : public BaseInst<FCmpInst> {
    friend BaseInst<FCmpInst>;

  public:
    // enum CmpOp {
    //     EQ,  // ==
    //     NE,  // !=
    //     GT,  // >
    //     GE,  // >=
    //     LT,  // <
    //     LE   // <=
    // };

  private:
    FCmpInst(Type *ty, CmpOp op, std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs, BasicBlock *bb);

  public:
    static std::shared_ptr<FCmpInst> create_fcmp(CmpOp op,
                                                 std::shared_ptr<Value> lhs,
                                                 std::shared_ptr<Value> rhs,
                                                 BasicBlock *bb) {
        return create(Type::get_int1_type(bb->get_module()), op, lhs, rhs, bb);
    }

    CmpOp get_cmp_op() { return cmp_op_; }

    virtual std::string print() override;
    virtual std::size_t get_hash() const override {
        std::size_t ret = this->get_base_hash();
        ret = ret * mod + static_cast<int>(cmp_op_);
        return ret;
    }

    virtual bool equal(const std::shared_ptr<Instruction> &rhs) const override {
        if (this->Instruction::equal(rhs) == false)
            return false;
        auto cmpinst = std::dynamic_pointer_cast<FCmpInst>(rhs);
        if (cmpinst == nullptr or cmp_op_ != cmpinst->cmp_op_)
            return false;
        return true;
    }
    virtual bool is_hash_able() const override { return true; }

  private:
    CmpOp cmp_op_;

    void assert_valid();
};

class SelectInst : public BaseInst<SelectInst> {
    friend BaseInst<SelectInst>;

  protected:
    SelectInst(std::shared_ptr<Value> test_bit, std::shared_ptr<Value> t, std::shared_ptr<Value> f, BasicBlock *bb);
    SelectInst(std::shared_ptr<Value> test_bit,
               std::shared_ptr<Value> t,
               std::shared_ptr<Value> f,
               Instruction::self_iterator *insert_before = nullptr);

  public:
    std::string print() override;
    static std::shared_ptr<SelectInst> create_select(std::shared_ptr<Value> test_bit,
                                                     std::shared_ptr<Value> t,
                                                     std::shared_ptr<Value> f,
                                                     BasicBlock *bb) {
        return create(test_bit, t, f, bb);
    }
    static std::shared_ptr<SelectInst> create_select(std::shared_ptr<Value> test_bit,
                                                     std::shared_ptr<Value> t,
                                                     std::shared_ptr<Value> f,
                                                     Instruction::self_iterator *insert_before = nullptr) {
        return create(test_bit, t, f, insert_before);
    }
};
class CallInst : public BaseInst<CallInst> {
    friend BaseInst<CallInst>;

  protected:
    CallInst(std::shared_ptr<Function> func, std::vector<std::shared_ptr<Value>> args, BasicBlock *bb);
    CallInst(std::shared_ptr<Function> func,
             std::vector<std::shared_ptr<Value>> args,
             self_iterator *insert_before = nullptr);

  public:
    // static std::shared_ptr<CallInst> create(std::shared_ptr<Function> func,
    //                                         std::vector<std::shared_ptr<Value>> args,
    //                                         BasicBlock *bb);
    FunctionType *get_function_type() const;
    Function *get_callee();
    virtual std::string print() override;

    virtual std::size_t get_hash() const override { return this->get_base_hash(); }
    virtual bool equal(const std::shared_ptr<Instruction> &rhs) const override { return this->Instruction::equal(rhs); }
};

class BranchInst : public BaseInst<BranchInst> {
    friend BaseInst<BranchInst>;

  private:
    BranchInst(std::shared_ptr<Value> cond,
               std::shared_ptr<BasicBlock> if_true,
               std::shared_ptr<BasicBlock> if_false,
               BasicBlock *bb);
    BranchInst(std::shared_ptr<BasicBlock> if_true, BasicBlock *bb);

  public:
    static std::shared_ptr<BranchInst> create_cond_br(std::shared_ptr<Value> cond,
                                                      std::shared_ptr<BasicBlock> if_true,
                                                      std::shared_ptr<BasicBlock> if_false,
                                                      BasicBlock *bb);
    static std::shared_ptr<BranchInst> create_br(std::shared_ptr<BasicBlock> if_true, BasicBlock *bb);

    bool is_cond_br() const;
    std::shared_ptr<BasicBlock> get_true_succ() {
        assert(is_cond_br() and "try to get true branch of a uncond br");
        return std::dynamic_pointer_cast<BasicBlock>(get_operand(1)->shared_from_this());
    }
    std::shared_ptr<BasicBlock> get_false_succ() {
        assert(is_cond_br() and "try to get false branch of a uncond br");
        return std::dynamic_pointer_cast<BasicBlock>(get_operand(2)->shared_from_this());
    }
    std::shared_ptr<BasicBlock> get_target() {
        assert(not is_cond_br() and "try to get false branch of a uncond br");
        return std::dynamic_pointer_cast<BasicBlock>(get_operand(0)->shared_from_this());
    }
    CmpOp op = CmpOp::NE;
    void set_cmp_op(CmpOp o) { op = o; }
    CmpOp get_cmp_op() { return op; }
    virtual std::string print() override;
};

class ReturnInst : public BaseInst<ReturnInst> {
    friend BaseInst<ReturnInst>;

  private:
    ReturnInst(std::shared_ptr<Value> val, BasicBlock *bb);
    ReturnInst(BasicBlock *bb);

  public:
    static std::shared_ptr<ReturnInst> create_ret(std::shared_ptr<Value> val, BasicBlock *bb) {
        return create(val, bb);
    }
    static std::shared_ptr<ReturnInst> create_void_ret(BasicBlock *bb) { return create(bb); }
    bool is_void_ret() const;

    virtual std::string print() override;
};

class GetElementPtrInst : public BaseInst<GetElementPtrInst> {
    friend BaseInst<GetElementPtrInst>;

  private:
    GetElementPtrInst(std::shared_ptr<Value> ptr, std::vector<std::shared_ptr<Value>> idxs, BasicBlock *bb);
    GetElementPtrInst(std::shared_ptr<Value> ptr,
                      std::vector<std::shared_ptr<Value>> idxs,
                      Instruction::self_iterator *insert_before = nullptr);
    GetElementPtrInst(Type *ty, int num_op, Instruction::self_iterator *insert_before = nullptr)
        : BaseInst<GetElementPtrInst>(ty, Instruction::getelementptr, num_op, insert_before) {}

    GetElementPtrInst(Type *ty, int num_op, BasicBlock *bb)
        : BaseInst<GetElementPtrInst>(ty, Instruction::getelementptr, num_op, bb) {}

  public:
    static Type *get_element_type(std::shared_ptr<Value> ptr, std::vector<std::shared_ptr<Value>> idxs);
    static std::shared_ptr<GetElementPtrInst> create_gep(std::shared_ptr<Value> ptr,
                                                         std::vector<std::shared_ptr<Value>> idxs,
                                                         BasicBlock *bb) {
        return create(ptr, idxs, bb);
    }
    Type *get_element_type() const;

    virtual std::string print() override;
    virtual std::size_t get_hash() const override {
        std::size_t ret = this->get_base_hash();
        ret = ret * mod + reinterpret_cast<uintptr_t>(element_ty_);
        return ret;
    }
    virtual bool is_hash_able() const override { return true; }

    virtual bool equal(const std::shared_ptr<Instruction> &rhs) const override {
        if (this->Instruction::equal(rhs) == false)
            return false;
        auto myinst = std::dynamic_pointer_cast<GetElementPtrInst>(rhs);
        if (myinst == nullptr or element_ty_ != myinst->element_ty_)
            return false;
        return true;
    }

  private:
    Type *element_ty_;
};

class StoreInst : public BaseInst<StoreInst> {
    friend BaseInst<StoreInst>;

  private:
    StoreInst(std::shared_ptr<Value> val, std::shared_ptr<Value> ptr, BasicBlock *bb);
    StoreInst(std::shared_ptr<Value> val, std::shared_ptr<Value> ptr, self_iterator *insert_before);
    int op2_sh_num = 0;
    shift_type_t op2_sh_type = shift_type_t::LSL;

  public:
    static std::shared_ptr<StoreInst> create_store(std::shared_ptr<Value> val,
                                                   std::shared_ptr<Value> ptr,
                                                   BasicBlock *bb) {
        return create(val, ptr, bb);
    }
    shift_type_t get_op2_shift_type() { return op2_sh_type; }
    int get_op2_shift_bits() { return op2_sh_num; }

    bool set_op2_shift(shift_type_t t, int n) {
        if (n == 0)
            op2_sh_type = shift_type_t::LSL;
        else
            op2_sh_type = t;
        if (op2_sh_type == shift_type_t::LSL && !(n >= 0 && n <= 31)) {
            LOG_ERROR << "illegal LSL number";
            return false;
        }
        if ((op2_sh_type == shift_type_t::LSR || op2_sh_type == shift_type_t::ASR) && !(n >= 1 && n <= 32)) {
            LOG_ERROR << "illegal (A/L)SR number";
            return false;
        }
        op2_sh_num = n;
        return true;
    }

    std::shared_ptr<Value> get_rval() { return std::get<0>(get_operand(0).operand); }
    std::shared_ptr<Value> get_lval() { return std::get<0>(get_operand(1).operand); }
    virtual std::string print() override;
};

class LoadInst : public BaseInst<LoadInst> {
    friend BaseInst<LoadInst>;

  private:
    LoadInst(Type *ty, std::shared_ptr<Value> ptr, BasicBlock *bb);
    LoadInst(Type *ty, std::shared_ptr<Value> ptr, self_iterator *insert_before = nullptr);
    int op2_sh_num = 0;
    shift_type_t op2_sh_type = shift_type_t::LSL;

  public:
    static std::shared_ptr<LoadInst> create_load(Type *ty, std::shared_ptr<Value> ptr, BasicBlock *bb) {
        return create(ty, ptr, bb);
    }
    shift_type_t get_op2_shift_type() { return op2_sh_type; }
    int get_op2_shift_bits() { return op2_sh_num; }

    bool set_op2_shift(shift_type_t t, int n) {
        if (n == 0)
            op2_sh_type = shift_type_t::LSL;
        else
            op2_sh_type = t;
        if (op2_sh_type == shift_type_t::LSL && !(n >= 0 && n <= 31)) {
            LOG_ERROR << "illegal LSL number";
            return false;
        }
        if ((op2_sh_type == shift_type_t::LSR || op2_sh_type == shift_type_t::ASR) && !(n >= 1 && n <= 32)) {
            LOG_ERROR << "illegal (A/L)SR number";
            return false;
        }
        op2_sh_num = n;
        return true;
    }
    // static std::shared_ptr<LoadInst> create_load(Type *ty, std::shared_ptr<Value> ptr);
    std::shared_ptr<Value> get_lval() { return std::get<0>(get_operand(0).operand); }

    Type *get_load_type() const;

    virtual std::string print() override;
};

class AllocaInst : public BaseInst<AllocaInst> {
    friend BaseInst<AllocaInst>;

  private:
    AllocaInst(Type *ty, BasicBlock *bb);
    AllocaInst(Type *ty, self_iterator *insert_before = nullptr);

  public:
    static std::shared_ptr<AllocaInst> create_alloca(Type *ty, BasicBlock *bb) { return create(ty, bb); }

    Type *get_alloca_type() const;

    virtual std::string print() override;

  private:
    Type *alloca_ty_;
};

class PtrToIntInst : public BaseInst<PtrToIntInst> {
    friend BaseInst<PtrToIntInst>;

  private:
    PtrToIntInst(std::shared_ptr<Value> value, BasicBlock *bb);
    PtrToIntInst(std::shared_ptr<Value> value, Instruction::self_iterator *insert_before = nullptr);

  public:
    static std::shared_ptr<PtrToIntInst> create_ptrtoint(std::shared_ptr<Value> value,
                                                         Instruction::self_iterator *insert_before = nullptr) {
        auto test = create(value, insert_before);
        return test;  // create(insert_before, value);
    }
    static std::shared_ptr<PtrToIntInst> create_ptrtoint(std::shared_ptr<Value> value, BasicBlock *bb) {
        return create(value, bb);
    }
    virtual bool is_hash_able() const override { return true; }
    virtual std::string print() override;
    virtual std::size_t get_hash() const override { return this->get_base_hash(); }
};

class IntToPtrInst : public BaseInst<IntToPtrInst> {
    friend BaseInst<IntToPtrInst>;

  private:
    IntToPtrInst(std::shared_ptr<Value> value, Type *to, BasicBlock *bb);
    IntToPtrInst(std::shared_ptr<Value> value, Type *to, Instruction::self_iterator *insert_before);

  public:
    static std::shared_ptr<IntToPtrInst> create_inttoptr(std::shared_ptr<Value> value, Type *to, BasicBlock *bb) {
        return create(value, to, bb);
    }
    static std::shared_ptr<IntToPtrInst> create_inttoptr(std::shared_ptr<Value> value,
                                                         Type *to,
                                                         Instruction::self_iterator *insert_before = nullptr) {
        return create(value, to, insert_before);
    }

    virtual bool is_hash_able() const override { return true; }
    virtual std::string print() override;
    virtual std::size_t get_hash() const override { return this->get_base_hash(); }
};
class ZextInst : public BaseInst<ZextInst> {
    friend BaseInst<ZextInst>;

  private:
    ZextInst(OpID op, std::shared_ptr<Value> val, Type *ty, BasicBlock *bb);
    ZextInst(OpID op, std::shared_ptr<Value> val, Type *ty, self_iterator * = nullptr);

  public:
    static std::shared_ptr<ZextInst> create_zext(std::shared_ptr<Value> val, Type *ty, BasicBlock *bb) {
        return create(Instruction::zext, val, ty, bb);
    }

    static std::shared_ptr<ZextInst> create_zext(std::shared_ptr<Value> val,
                                                 Type *ty,
                                                 self_iterator *insert_before = nullptr) {
        return create(Instruction::zext, val, ty, insert_before);
    }

    Type *get_dest_type() const;

    virtual std::string print() override;
    virtual bool is_hash_able() const override { return true; }
    virtual std::size_t get_hash() const override {
        std::size_t ret = this->get_base_hash();
        ret = ret * mod + reinterpret_cast<uintptr_t>(dest_ty_);
        return ret;
    }

    virtual bool equal(const std::shared_ptr<Instruction> &rhs) const override {
        if (this->Instruction::equal(rhs) == false)
            return false;
        auto myinst = std::dynamic_pointer_cast<ZextInst>(rhs);
        if (myinst == nullptr or dest_ty_ != myinst->dest_ty_)
            return false;
        return true;
    }

  private:
    Type *dest_ty_;
};

class FpToSiInst : public BaseInst<FpToSiInst> {
    friend BaseInst<FpToSiInst>;

  private:
    FpToSiInst(OpID op, std::shared_ptr<Value> val, Type *ty, BasicBlock *bb);

  public:
    static std::shared_ptr<FpToSiInst> create_fptosi(std::shared_ptr<Value> val, Type *ty, BasicBlock *bb) {
        return create(Instruction::fptosi, val, ty, bb);
    }

    Type *get_dest_type() const;

    virtual std::string print() override;
    virtual bool is_hash_able() const override { return true; }
    virtual std::size_t get_hash() const override {
        std::size_t ret = this->get_base_hash();
        ret = ret * mod + reinterpret_cast<uintptr_t>(dest_ty_);
        return ret;
    }

    virtual bool equal(const std::shared_ptr<Instruction> &rhs) const override {
        if (this->Instruction::equal(rhs) == false)
            return false;
        auto myinst = std::dynamic_pointer_cast<FpToSiInst>(rhs);
        if (myinst == nullptr or dest_ty_ != myinst->dest_ty_)
            return false;
        return true;
    }

  private:
    Type *dest_ty_;
};

class SiToFpInst : public BaseInst<SiToFpInst> {
    friend BaseInst<SiToFpInst>;

  private:
    SiToFpInst(OpID op, std::shared_ptr<Value> val, Type *ty, BasicBlock *bb);

  public:
    static std::shared_ptr<SiToFpInst> create_sitofp(std::shared_ptr<Value> val, Type *ty, BasicBlock *bb) {
        return create(Instruction::sitofp, val, ty, bb);
    }

    Type *get_dest_type() const;

    virtual std::string print() override;
    virtual bool is_hash_able() const override { return true; }
    virtual std::size_t get_hash() const override {
        std::size_t ret = this->get_base_hash();
        ret = ret * mod + reinterpret_cast<uintptr_t>(dest_ty_);
        return ret;
    }

    virtual bool equal(const std::shared_ptr<Instruction> &rhs) const override {
        if (this->Instruction::equal(rhs) == false)
            return false;
        auto myinst = std::dynamic_pointer_cast<SiToFpInst>(rhs);
        if (myinst == nullptr or dest_ty_ != myinst->dest_ty_)
            return false;
        return true;
    }

  private:
    Type *dest_ty_;
};

class SwitchInst : public BaseInst<SwitchInst> {
    friend BaseInst<SwitchInst>;

  private:
    SwitchInst(OpID op,
               std::shared_ptr<Value> val,
               std::shared_ptr<BasicBlock> default_block,
               std::vector<std::shared_ptr<Value>> vals,
               std::vector<std::shared_ptr<BasicBlock>> val_bbs,
               Type *ty,
               BasicBlock *bb);
    SwitchInst(OpID op,
               std::shared_ptr<Value> val,
               std::shared_ptr<BasicBlock> default_block,
               std::vector<std::shared_ptr<Value>> vals,
               std::vector<std::shared_ptr<BasicBlock>> val_bbs,
               Type *ty,
               Instruction::self_iterator *insert_before = nullptr);
    SwitchInst(Type *ty, OpID op, std::shared_ptr<Value> val, std::shared_ptr<BasicBlock> default_block, BasicBlock *bb)
        : BaseInst<SwitchInst>(ty, op, 2, bb), test_val(val), default_block(default_block) {
        set_operand(0, val);
        set_operand(1, std::weak_ptr<Value>(default_block));
    }
    std::shared_ptr<Value> test_val;
    std::shared_ptr<BasicBlock> default_block;

  public:
    static std::shared_ptr<SwitchInst> create_switch(std::shared_ptr<Value> val,
                                                     std::shared_ptr<BasicBlock> default_block,
                                                     BasicBlock *bb) {
        return create(OpID::switch1,
                      val,
                      default_block,
                      std::vector<std::shared_ptr<Value>>{},
                      std::vector<std::shared_ptr<BasicBlock>>{},
                      Type::get_void_type(default_block->get_module()),
                      bb);
    }
    static std::shared_ptr<SwitchInst> create_switch(std::shared_ptr<Value> val,
                                                     std::shared_ptr<BasicBlock> default_block,
                                                     Instruction::self_iterator *insert_before = nullptr) {
        return create(OpID::switch1,
                      val,
                      default_block,
                      std::vector<std::shared_ptr<Value>>{},
                      std::vector<std::shared_ptr<BasicBlock>>{},
                      Type::get_void_type(default_block->get_module()),
                      insert_before);
    }
    void add_switch_pair(std::shared_ptr<Value> val, std::shared_ptr<BasicBlock> bb) {
        add_operand(val);
        add_operand(std::weak_ptr<Value>(bb));
    }
    virtual std::string print() override;
    BasicBlock *get_default_block() const { return default_block.get(); }
};

class PhiInst : public BaseInst<PhiInst> {
    friend BaseInst<PhiInst>;

  private:
    PhiInst(OpID op,
            std::vector<std::shared_ptr<Value>> vals,
            std::vector<std::shared_ptr<BasicBlock>> val_bbs,
            Type *ty,
            BasicBlock *bb);
    PhiInst(OpID op,
            std::vector<std::shared_ptr<Value>> vals,
            std::vector<std::shared_ptr<BasicBlock>> val_bbs,
            Type *ty,
            Instruction::self_iterator *insert_before = nullptr);
    PhiInst(Type *ty, OpID op, unsigned num_ops, BasicBlock *bb) : BaseInst<PhiInst>(ty, op, num_ops, bb) {}
    std::shared_ptr<Value> l_val_;

  public:
    static std::shared_ptr<PhiInst> create_phi(Type *ty, BasicBlock *bb) {
        // why can't just use {}?
        std::vector<std::shared_ptr<Value>> vals;
        std::vector<std::shared_ptr<BasicBlock>> bbs;
        return create(Instruction::phi, vals, bbs, ty, bb);
    }
    static std::shared_ptr<PhiInst> create_phi(Type *ty, Instruction::self_iterator *insert_before = nullptr) {
        std::vector<std::shared_ptr<Value>> vals;
        std::vector<std::shared_ptr<BasicBlock>> bbs;
        return create(Instruction::phi, vals, bbs, ty, insert_before);
    }
    std::shared_ptr<Value> get_lval() { return l_val_; }
    void set_lval(std::shared_ptr<Value> l_val) { l_val_ = l_val; }
    void add_phi_pair_operand(std::shared_ptr<Value> val, std::shared_ptr<BasicBlock> pre_bb) {
        this->add_operand(val);
        this->add_operand(std::weak_ptr<Value>(pre_bb));
    }
    void add_phi_pair_operand(std::weak_ptr<Value> val, std::shared_ptr<BasicBlock> pre_bb) {
        LOG_DEBUG<<"phi set operand";
        if (dynamic_cast<Constant *>(val.lock().get()))
            this->add_operand(std::shared_ptr<Value>(val));
        else
            this->add_operand(val);
        this->add_operand(std::weak_ptr<Value>(pre_bb));
    }
    std::shared_ptr<Value> input_of(BasicBlock *bb) {
        for (size_t i = 1; i < this->get_num_operand(); i += 2) {
            auto input = this->get_operand(i).get();
            if (input == bb)
                return this->get_operand(i - 1)->shared_from_this();
        }
        LOG_DEBUG << "no corresponding operand";
        return nullptr;
    }
    void remove_bb(std::shared_ptr<BasicBlock> bb) {
        for (int i = 0; i < this->get_num_operand(); i += 2) {
            auto input = std::dynamic_pointer_cast<BasicBlock>(this->get_operand(i + 1)->shared_from_this());
            if (input == bb) {
                remove_operands(i, i + 1);
                return;
            }
        }
    }
    virtual std::string print() override;
    virtual bool is_hash_able() const override { return true; }
    virtual std::size_t get_hash() const override { return this->get_base_hash(); }
};

#endif  // SYSYC_INSTRUCTION_H
