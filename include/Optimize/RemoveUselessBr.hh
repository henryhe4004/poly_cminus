#ifndef REMOVE_USELESS_BR_HH
#define REMOVE_USELESS_BR_HH

#include "Pass.hh"

class RemoveUselessBr : public Pass {
  public:
    RemoveUselessBr(Module *m) : Pass(m) {}

    void run();
};

#endif
