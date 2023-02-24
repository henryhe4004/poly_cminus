#pragma once
#include "BasicBlock.h"
#include "Pass.hh"

class Branch : public Pass {
  public:
    Branch(Module *m) : Pass(m) {}
    void run();

  private:
    enum pattern { land, lor, one_each, unknown };
    std::list<std::tuple<Value *, BasicBlock *, BasicBlock *>> identity_condition_blocks(BasicBlock *, bool lhs_same);
    bool try_optimize_jump_table(Function *func);
    bool try_optimize_phi(Function* func);
    bool try_optimize_and_or(Function * func);
    pattern analyze(const std::list<std::tuple<Value *, BasicBlock *, BasicBlock *>> &);
    void jump_table();
    void replace_phi_with_select();
    void replace_and_or();
};
