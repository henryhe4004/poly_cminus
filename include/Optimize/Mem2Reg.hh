#ifndef MEM2REG_HH
#define MEM2REG_HH

#include "Dominators.hh"

using std::shared_ptr;
using std::unique_ptr;

class Mem2Reg : public Pass {
  public:
    Mem2Reg(Module *m) : Pass(m) {}
    void run();

  private:
    unique_ptr<Dominators> dominators;

    void generate_phi(Function *function);
    void rename(BasicBlock *bb);
    void remove_alloca(Function *function);
};

#endif
