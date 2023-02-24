#pragma once
#include "Instruction.h"
#include "LoopSearch.hh"
#include "Module.h"
#include "Pass.hh"
#include "utils.hh"

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using std::shared_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

class Loop {
    friend class LoopInfo;

  public:
    // Loop() = default;
    Loop(bb_set_t &bbs, Module *m)
        : blocks(bbs)
        , parent_loop(nullptr)
        , base(nullptr)
        , preheader(nullptr)
        , latch(nullptr)
        , end_block(nullptr)
        , exit(nullptr)
        , m_(m) {}
    // 描述归纳变量信息的类
    struct InductionVar {
        // 对应的指令
        shared_ptr<Instruction> inst;
        // 归纳变量类型，基本or依赖
        enum class IndVarType { basic, dependent } type;
        // 方向
        enum class direction_t { increasing, decreasing, unknown } direction = direction_t::unknown;
        CmpOp predicate;
        // 边界的谓词，i <= 100、i < 100、i > 0等
        // 修改归纳变量的二元运算指令的op，一般为add或sub
        Instruction::OpID bin_op;
        // 对于依赖归纳变量，其基
        shared_ptr<InductionVar> basis;
        // 初始值
        shared_ptr<Value> initial_val;
        // 终止值，可能为空指针
        shared_ptr<Value> final_val;
        // 步长
        shared_ptr<Value> step_val;
        // 使归纳变量增加或减少一个步长的指令
        shared_ptr<Instruction> step_inst;
        // 获得初始值的常数值（如果是的话）
        std::optional<int> get_const_init_val() { return utils::get_const_int_val(initial_val.get()); }
        std::optional<int> get_const_step_val() { return utils::get_const_int_val(step_val.get()); }
        std::optional<int> get_const_final_val() { return utils::get_const_int_val(final_val.get()); }
        static std::optional<int> get_const_int_val(Value *v) {
            if (auto c = dynamic_cast<ConstantInt *>(v)) {
                return c->get_value();
            }
            return std::nullopt;
        }
        // j = ci * i + c2, i is a basic induction variable
        shared_ptr<Value> c1;
        shared_ptr<Value> c2;
        // enum class sign_t { pos, neg } c1_sign, c2_sign;
        InductionVar() : predicate(CmpOp::EQ) {}
        InductionVar(shared_ptr<Instruction> i_, IndVarType t_) : inst(i_), type(t_), predicate(CmpOp::EQ) {}
    };
    /// while(cond)
    ///   <loop body>
    /// --- pseudo LLVMIR ---
    /// beforeloop:
    ///   if (cond) goto preheader; else goto afterloop
    /// preheader:
    /// loop:
    ///   <loop body>
    /// latch:
    ///   if (cond) goto loop; else goto exit
    /// exit:
    /// afterloop:

    // 获得循环的基本块集合
    bb_set_t &get_blocks() { return blocks; }
    Loop *get_parent_loop() { return parent_loop; }
    vector<Loop *> &get_sub_loops() { return sub_loops; }
    // 循环入口
    shared_ptr<BasicBlock> get_base() { return base; }
    // 获得preheader，即循环入口的前一个块
    shared_ptr<BasicBlock> get_preheader() { return preheader; }
    // 最后一个基本块，精确地说，是循环入口的两个前驱中在循环中的那一个
    shared_ptr<BasicBlock> get_end() { return end_block; }
    // latch的起始块，即循环尾部进行循环条件判断的一系列基本块的第一个
    shared_ptr<BasicBlock> get_latch() { return latch; }
    // 循环退出后的第一个块
    shared_ptr<BasicBlock> get_exit() { return exit; }
    // 循环归纳变量列表
    unordered_set<shared_ptr<InductionVar>> &get_ind_vars() { return ind_vars; }
    auto insert_IV(shared_ptr<InductionVar> iv) { return ind_vars.insert(iv); }
    auto find_IV(shared_ptr<InductionVar> iv) { return ind_vars.find(iv); }
    auto IV_end() { return ind_vars.end(); }
    auto get_bound_IV() { return bound_IV; }
    shared_ptr<InductionVar> IV_exists(Instruction *I) {
        if (auto f = inst2iv.find(I); f != inst2iv.end()) {
            return f->second;
        } else
            return nullptr;
    }
    // 判断给定Value是否为循环不变量
    bool is_loop_invariant(Value *);
    bool lazy_invariant(Value *v) {
        if (auto inst = dynamic_cast<Instruction *>(v))
            return !contains(inst);
        return true;
    }
    // 判断循环是否包含给定块/指令
    bool contains(BasicBlock *B) { return blocks.find(B) != blocks.end(); }
    bool contains(Instruction *I) { return contains(I->get_parent()); }

    // 在循环preheader的跳转指令之前插入指令
    auto insert_into_preheader(shared_ptr<Instruction> I) {
        auto ter = preheader->get_instructions().end();
        ter--;
        I->set_parent(preheader.get());
        return preheader->get_instructions().insert(ter, I);
    }
    void insert_into_preheader(shared_ptr<Value> v) {
        if (auto inst = std::dynamic_pointer_cast<Instruction>(v))
            insert_into_preheader(inst);
    }

    // 在循环开始处插入phi指令
    auto insert_phi_at_begin(shared_ptr<PhiInst> phi) {
        phi->set_parent(base.get());
        return base->add_instr_begin(phi);
    }

    // 在base块的phi指令之后插入指令
    void insert_at_begin(shared_ptr<Instruction> I) {
        for (auto itr = base->get_instructions().begin(); itr != base->get_instructions().end(); itr++) {
            if (!(*itr)->is_phi()) {
                base->get_instructions().insert(itr, I);
                I->set_parent(base.get());
                return;
            }
        }
    }

    // 返回从当前循环开始，子循环的最大嵌套深度
    // 没有子循环时返回1
    int get_sub_depth() {
        int sub_depth = 0;
        for (auto sub : get_sub_loops()) {
            auto d = sub->get_sub_depth();
            if (d > sub_depth)
                sub_depth = d;
        }
        return sub_depth + 1;
    }

    void find_basic_IV();
    void find_dependent_IV();
    void find_all_IV() {
        find_basic_IV();
        find_dependent_IV();
    }
    bool has_break_etmt() { return has_break_; }

  private:
    bb_set_t blocks;
    Loop *parent_loop;
    vector<Loop *> sub_loops;
    unordered_set<shared_ptr<InductionVar>> ind_vars;
    shared_ptr<BasicBlock> base;
    shared_ptr<BasicBlock> preheader;
    shared_ptr<BasicBlock> latch;
    shared_ptr<BasicBlock> end_block;
    shared_ptr<BasicBlock> exit;
    // bb_set_t latch_set;
    unordered_map<Instruction *, shared_ptr<InductionVar>> inst2iv;
    bb_set_t latch_blocks;
    shared_ptr<InductionVar> bound_IV;
    bool has_break_ = false;

    void find_pre_header_and_end();
    void find_latch();
    void find_bound();
    void find_break();
    Module *m_;
};
using ivptr = shared_ptr<Loop::InductionVar>;
using ivtype = Loop::InductionVar::IndVarType;
class LoopInfo : public Pass {
  public:
    LoopInfo(Module *m) : Pass(m), loop_search_(m) {}
    void run();
    // IndVarList &get_loop_ind(Loop&);
    static bool is_constant(Value *v) {
        if (dynamic_cast<Constant *>(v))
            return true;
        return false;
    }
    vector<Loop> &get_loops() { return loops; }
    auto begin() { return loops.begin(); }
    auto end() { return loops.end(); }
    Loop *get_inner_loop(BasicBlock *bb) {
        auto inner_bbset = loop_search_.get_inner_loop(bb);
        if (!inner_bbset)
            return nullptr;
        return &loops[bbset_ptr_to_loop_idx[inner_bbset]];
    }

  private:
    // vector<shared_ptr<InductionVar>> ind_vars;
    loop_search loop_search_;
    // std::unordered_map<shared_ptr<bb_set_t>, Loop> loop_info;
    vector<Loop> loops;
    std::unordered_map<shared_ptr<bb_set_t>, int> bbset_ptr_to_loop_idx;
    void test_print();
    // void find_ind(Loop &);
    // void find_pre_header_and_exit(Loop &);
};
