#include "Function.h"
#include "Pass.hh"
#include "utils.hh"

#include <unordered_map>

class FuncInline : public Pass {
  public:
    FuncInline(Module *m) : Pass(m), MAX_INSTRUCTION_NUM(100) {}
    void run();
    bool is_inlineable(Function *func);

  private:
    const int MAX_INSTRUCTION_NUM;
    int counter = 0;

    // 原函数中的每一个Value（基本块，指令，形参等）到内联后的对应的新Value的映射
    std::unordered_map<Value *, Value *> val_old2new;
    Value *get_new_val(Value *old) {
        if (utils::is_const(old) || utils::is_global_var(old))
            return old;
        auto new_val = val_old2new.find(old);
        if (new_val == val_old2new.end()) {
            exit(201);
            LOG_ERROR << "new value of " << old->get_name() << " has not defined";
            return nullptr;
        }
        return new_val->second;
    }
    Value *get_new_op(shared_ptr<Instruction> old_inst, int i) {
        if (i >= old_inst->get_num_operand()) {
            exit(202);
            LOG_ERROR << "operand index out of range";
        }
        return get_new_val(old_inst->get_operand(i).get());
    }
};
