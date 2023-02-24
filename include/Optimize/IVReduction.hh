#pragma once
#include "Instruction.h"
#include "LoopInfo.hh"
#include "Module.h"
#include "Pass.hh"

#include <unordered_map>
#include <unordered_set>

class IVReduction : public Pass {
  public:
    IVReduction(Module *m) : Pass(m) {}
    void run();

  private:
    void dependent_IV_red();
    void IV_final_val();
    void common_iv_elim();
    // LoopInfo loop_info;
};