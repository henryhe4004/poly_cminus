#pragma once
#include "Pass.hh"

#include <map>
#include <memory>

class GepElimination : public Pass {
  public:
    GepElimination(Module *m) : Pass(m) {}
    void run() override;

  private:
    std::shared_ptr<BasicBlock> entry;
    void remove_gep(BasicBlock *);
    std::map<Value *, std::shared_ptr<Value>> addr;
};
