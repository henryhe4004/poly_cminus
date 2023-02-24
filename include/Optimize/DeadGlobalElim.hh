#pragma once
#include "Pass.hh"

#include <algorithm>

// remove global array/vars with only stores or loads, as in perf test sl1
class DeadGlobalElimination : public Pass {
  public:
    DeadGlobalElimination(Module *m) : Pass(m) {}
    void run() override;
};
