#pragma once
#include "Function.h"
#include "Pass.hh"
#include "errorcode.hh"
#include "logging.hpp"

#include <deque>
#include <unordered_map>
#include <unordered_set>

/**
 * 计算哪些函数是纯函数
 * WARN: 假定所有函数都是纯函数，除非他写入了全局变量、修改了传入的数组、或者直接间接调用了非纯函数
 */
class FuncInfo : public Pass {
  public:
    FuncInfo(Module *m) : Pass(m) {}

    void run() {
        for (auto func : m_->get_functions()) {
            trivial_mark(func);
            if (not is_pure[func])
                worklist.push_back(func);
        }
        while (worklist.empty() == false) {
            auto now = worklist.front();
            worklist.pop_front();
            process(now);
        }
        log();
    }

    bool is_pure_function(std::shared_ptr<Function> func) {
        exit_if(is_pure.find(func) == is_pure.end(), ERROR_IN_PURE_FUNCTION_ANALYSIS);
        return is_pure[func];
    }

    void log() {
        for (auto it : is_pure) {
            LOG_INFO << it.first->get_name() << " is pure? " << it.second;
        }
    }

  private:
    // 有 store 操作的函数非纯函数来处理
    void trivial_mark(std::shared_ptr<Function> func) {
        if (func->is_declaration() or func->get_name() == "main") {
            is_pure[func] = false;
            return;
        }
        // 哪怕传入数组，只要没有 store 操作，也可以作为纯函数处理
        // 还是当做不行来处理了
        for (auto it = func->get_function_type()->param_begin(); it != func->get_function_type()->param_end(); ++it) {
            auto arg_type = *it;
            if (arg_type->is_integer_type() == false and arg_type->is_float_type() == false) {
                is_pure[func] = false;
                return;
            }
        }
        for (auto bb : func->get_basic_blocks())
            for (auto inst : bb->get_instructions()) {
                if (is_side_effect_inst(inst)) {
                    is_pure[func] = false;
                    return;
                }
            }
        is_pure[func] = true;
    }

    void process(std::shared_ptr<Function> func) {
        for (auto &use : func->get_use_list()) {
            LOG_INFO << use.val_->print() << " uses func: " << func->get_name();
            if (auto inst = std::dynamic_pointer_cast<Instruction>(use.val_->shared_from_this())) {
                auto func = std::dynamic_pointer_cast<Function>(inst->get_parent()->get_parent()->shared_from_this());
                if (is_pure[func]) {
                    is_pure[func] = false;
                    worklist.push_back(func);
                }
            } else
                LOG_WARNING << "Value besides instruction uses a function";
        }
    }

    // DONE 对局部变量进行 store 应该也是没有副作用的，这部分可以进一步完善
    bool is_side_effect_inst(std::shared_ptr<Instruction> inst) {
        if (inst->is_store()) {
            if (is_local_store(std::dynamic_pointer_cast<StoreInst>(inst)))
                return false;
            return true;
        }
        if (inst->is_load()) {
            if (is_local_load(std::dynamic_pointer_cast<LoadInst>(inst)))
                return false;
            return true;
        }
        // call 指令的副作用会在后续 bfs 中计算
        return false;
    }

    bool is_local_load(std::shared_ptr<LoadInst> inst) {
        auto addr = std::dynamic_pointer_cast<Instruction>(get_first_addr(inst->get_operand(0)->shared_from_this()));
        if (addr and addr->is_alloca())
            return true;
        return false;
    }

    bool is_local_store(std::shared_ptr<StoreInst> inst) {
        auto addr = std::dynamic_pointer_cast<Instruction>(get_first_addr(inst->get_lval()));
        if (addr and addr->is_alloca())
            return true;
        return false;
    }
    std::shared_ptr<Value> get_first_addr(std::shared_ptr<Value> val) {
        if (auto inst = std::dynamic_pointer_cast<Instruction>(val)) {
            if (inst->is_alloca())
                return inst;
            if (inst->is_gep())
                return get_first_addr(inst->get_operand(0)->shared_from_this());
            if (inst->is_load())
                return val;
            LOG_WARNING << "FuncInfo: try to determine addr in operands";
            for (auto op : inst->get_operands()) {
                if (op->get_type()->is_pointer_type())
                    return get_first_addr(op->shared_from_this());
            }
        }
        return val;
    }

    std::deque<std::shared_ptr<Function>> worklist;
    std::unordered_map<std::shared_ptr<Function>, bool> is_pure;
};