#include "Pass.hh"
#include "unordered_set"

class GloVarLocal : public Pass {
  public:
    GloVarLocal(Module *m) : Pass(m) {}
    void run();

  private:
    void remove_uncalled_func();
    // void sort_blocks();
};