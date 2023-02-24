#pragma once
#include "BasicBlock.h"
#include "Pass.hh"
class Peephole : public Pass {
  public:
    Peephole(Module *m) : Pass(m) {}
    void run();

  private:
    void run_peephole(BasicBlock *);
    void run_peephole_aa(BasicBlock *);
};