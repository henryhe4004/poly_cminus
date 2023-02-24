#ifndef SYSYC_SYSYBUILDER_HPP
#define SYSYC_SYSYBUILDER_HPP

#include "AST.hpp"
#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "GlobalVariable.h"
#include "IRBuilder.h"
#include "Instruction.h"
#include "Module.h"
#include "Type.h"
#include "User.h"
#include "Value.h"

#include <map>

class Scope {
  public:
    // enter a new scope
    void enter() {
        inner.push_back({});
        array_param.push_back({});
    }

    // exit a scope
    void exit() {
        inner.pop_back();
        array_param.pop_back();
    }

    bool in_global() { return inner.size() == 1; }

    // push a name to scope
    // return true if successful
    // return false if this name already exits
    bool push(std::string name, std::shared_ptr<Value> val) {
        auto result = inner[inner.size() - 1].insert({name, val});
        return result.second;
    }

    bool push_params(std::string name, std::shared_ptr<Value> val, std::vector<std::shared_ptr<Value>> params) {
        auto result = array_param[array_param.size() - 1].insert({name, params});
        return result.second;
    }

    std::shared_ptr<Value> find(std::string name) {
        for (auto s = inner.rbegin(); s != inner.rend(); s++) {
            auto iter = s->find(name);
            if (iter != s->end()) {
                return iter->second;
            }
        }
        return nullptr;
    }

    std::shared_ptr<Value> find_params(std::string name, std::vector<std::shared_ptr<Value>> &params) {
        for (auto s = array_param.rbegin(); s != array_param.rend(); s++) {
            auto iter = s->find(name);
            if (iter != s->end()) {
                params.assign(iter->second.begin(), iter->second.end());
                return iter->second[0];
            }
        }
        return nullptr;
    }

  private:
    std::vector<std::map<std::string, std::shared_ptr<Value>>> inner;
    std::vector<std::map<std::string, std::vector<std::shared_ptr<Value>>>> array_param;
};

class SYSYCBuilder : public ASTVisitor {
  public:
    // TODO: add lib functions
    SYSYCBuilder() : module(new Module("Sysyc builder")), builder(new IRBuilder(nullptr, module.get())) {
        scope.enter();
        auto void_ty = Type::get_void_type(&*module);
        auto int32_ty = Type::get_int32_type(&*module);
        auto int32_ptr_ty = Type::get_int32_ptr_type(&*module);
        auto float_ty = Type::get_float_type(&*module);
        auto float_ptr_ty = Type::get_float_ptr_type(&*module);

        auto getint_type = FunctionType::get(int32_ty, {});
        auto getarray_type = FunctionType::get(int32_ty, {int32_ptr_ty});
        auto getfloat_type = FunctionType::get(float_ty, {});
        auto getfarray_type = FunctionType::get(int32_ty, {float_ptr_ty});
        auto putint_type = FunctionType::get(void_ty, {int32_ty});
        auto putch_type = FunctionType::get(void_ty, {int32_ty});
        auto putarray_type = FunctionType::get(void_ty, {int32_ty, int32_ptr_ty});
        auto putfloat_type = FunctionType::get(void_ty, {float_ty});
        auto putfarray_type = FunctionType::get(void_ty, {int32_ty, float_ptr_ty});
        auto starttime_type = FunctionType::get(void_ty, {int32_ty});
        auto stoptime_type = FunctionType::get(void_ty, {int32_ty});
        auto memset_type = FunctionType::get(void_ty, {int32_ptr_ty, int32_ty, int32_ty});
        auto memcpy_arm_type = FunctionType::get(void_ty, {int32_ptr_ty, int32_ptr_ty, int32_ty});

        auto getint_fun = Function::create(getint_type, "getint", module.get());
        auto getch_fun = Function::create(getint_type, "getch", module.get());
        auto getarray_fun = Function::create(getarray_type, "getarray", module.get());
        auto getfloat_fun = Function::create(getfloat_type, "getfloat", module.get());
        auto getfarray_fun = Function::create(getfarray_type, "getfarray", module.get());
        auto putint_fun = Function::create(putint_type, "putint", module.get());
        auto putch_fun = Function::create(putch_type, "putch", module.get());
        auto putarray_fun = Function::create(putarray_type, "putarray", module.get());
        auto putfloat_fun = Function::create(putfloat_type, "putfloat", module.get());
        auto putfarray_fun = Function::create(putfarray_type, "putfarray", module.get());
        auto starttime_fun = Function::create(starttime_type, "_sysy_starttime", module.get());
        auto stoptime_fun = Function::create(stoptime_type, "_sysy_stoptime", module.get());
        auto memset_fun = Function::create(memset_type, "memset32", module.get());
        auto memcpy_arm_fun = Function::create(memcpy_arm_type, "memcpy_arm", module.get());

        // TODO: putf?
        scope.push("getint", getint_fun);
        scope.push("getch", getch_fun);
        scope.push("getarray", getarray_fun);
        scope.push("getfloat", getfloat_fun);
        scope.push("getfarray", getfarray_fun);
        scope.push("putint", putint_fun);
        scope.push("putch", putch_fun);
        scope.push("putarray", putarray_fun);
        scope.push("putfloat", putfloat_fun);
        scope.push("putfarray", putfarray_fun);
        scope.push("_sysy_starttime", starttime_fun);
        scope.push("_sysy_stoptime", stoptime_fun);
        scope.push("memset32", memset_fun);
        scope.push("memcpy_arm", memcpy_arm_fun);
    }

    void IRprint() { std::cout << module->print(); }
    std::shared_ptr<Value> convert(std::shared_ptr<Value>, Type *);
    enum class bin_op { add, sub, mul, div, mod };
    std::shared_ptr<Value> calculate(std::shared_ptr<Value>, std::shared_ptr<Value>, bin_op);
    std::shared_ptr<Constant> build_up_initializer(std::vector<std::shared_ptr<Constant>>);

    Module *getModule() { return module.get(); }
    virtual void visit(ASTCompUnit &) override final;
    virtual void visit(ASTConstDecl &) override final;
    virtual void visit(ASTConstDef &) override final;
    virtual void visit(ASTConstInitSingle &) override final;
    virtual void visit(ASTConstInitList &) override final;
    virtual void visit(ASTVarDecl &) override final;
    virtual void visit(ASTVarDef &) override final;
    virtual void visit(ASTInitSingle &) override final;
    virtual void visit(ASTInitList &) override final;
    virtual void visit(ASTFuncDef &) override final;
    virtual void visit(ASTFuncFParam &) override final;
    virtual void visit(ASTBlock &) override final;
    virtual void visit(ASTBlockItem &) override final;
    virtual void visit(ASTAssignStmt &) override final;
    virtual void visit(ASTExpStmt &) override final;
    virtual void visit(ASTSelectionStmt &) override final;
    virtual void visit(ASTWhileStmt &) override final;
    virtual void visit(ASTBreakStmt &) override final;
    virtual void visit(ASTContinueStmt &) override final;
    virtual void visit(ASTReturnStmt &) override final;
    virtual void visit(ASTForStmt &) override final;
    virtual void visit(ASTOpUnaryExp &) override final;
    virtual void visit(ASTAddExp &) override final;
    virtual void visit(ASTMulExp &) override final;
    virtual void visit(ASTCall &) override final;
    virtual void visit(ASTLVal &) override final;
    virtual void visit(ASTNumber &) override final;
    virtual void visit(ASTLOrExp &) override final;
    virtual void visit(ASTLAndExp &) override final;
    virtual void visit(ASTEqExp &) override final;
    virtual void visit(ASTRelExp &) override final;
    virtual void visit(ASTConstExp &) override final;

  private:
    Scope scope;
    std::unique_ptr<Module> module;
    std::unique_ptr<IRBuilder> builder;
    std::list<std::shared_ptr<AllocaInst>> alloca_instructions;
};
#endif  // SYSYC_SYSYBUILDERLLVM_HPP
