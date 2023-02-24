#pragma once
#include "Instruction.h"
#include "Pass.hh"
class TailRecursionElim : public Pass {
    Function *f_;
    std::shared_ptr<BasicBlock> preheader, latch, header, return_bb;
    std::vector<std::shared_ptr<PhiInst>> phi_args, args_latch;
    Instruction *ret_phi;
    std::shared_ptr<PhiInst> acc_phi, acc_latch;
  public:
    TailRecursionElim(Module *m) : Pass(m) {}
    void run() override;
    CallInst *get_candidate(BasicBlock *);
    void create_header();
    bool eliminate_call(CallInst *);
    bool check_tre(CallInst *);
};