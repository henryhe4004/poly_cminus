#ifndef __LOOP_UNROLLING_HH__
#define __LOOP_UNROLLING_HH__

#include "Dominators.hh"
#include "LoopInfo.hh"
#include "LoopSearch.hh"
#include "Pass.hh"

#include <vector>

class LoopUnrolling : public Pass {
  public:
    LoopUnrolling(Module *m) : Pass(m), _loop_info(m) {}
    void run();

  private:
    LoopInfo _loop_info;
};

#endif
