#pragma once
#include "Dominators.hh"
#include "Pass.hh"
class InstructionSimplify : public Pass {
    std::unique_ptr<Dominators> dom;

  public:
    InstructionSimplify(Module *m) : Pass(m) {}

    void run() override;
    std::shared_ptr<Value> simplify_phi(std::shared_ptr<PhiInst>);
    bool dominates(std::shared_ptr<Value>, std::shared_ptr<Instruction>);
};