#ifndef __LOOPMERGE_HH__
#define __LOOPMERGE_HH__

#include "LoopInfo.hh"
#include "LoopSearch.hh"
#include "Pass.hh"

class LoopMerge : public Pass {
  public:
    LoopMerge(Module *m) : Pass(m), _loop_info(m) {}
    void run();

  private:
    LoopInfo _loop_info;
};

#endif
