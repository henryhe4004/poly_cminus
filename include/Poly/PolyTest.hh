#pragma once
#include "Pass.hh"
class PolyTest : public Pass {
  public:
    PolyTest(Module *m) : Pass(m) {}
    void run() override;
};
