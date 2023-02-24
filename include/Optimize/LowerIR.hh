#pragma once
#include "Pass.hh"

class LowerIR : public Pass {
  public:
    LowerIR(Module *m) : Pass(m) {}
    void run();

  private:
    void fuse_memory_offset();
    void fuse_mul_add();
    void fuse_const_shift();
    void delete_ptr_int_conversions();
    void fuse_br();
};
