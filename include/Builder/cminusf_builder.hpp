#ifndef _CMINUSF_BUILDER_HPP_
#define _CMINUSF_BUILDER_HPP_
#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"
#include "ast.hpp"
#include "User.h"
#include "Value.h"
#include "Instruction.h"

#include <map>
#include <memory>

class Scope {
  public:
    // enter a new scope
    void enter() { inner.push_back({}); }

    // exit a scope
    void exit() { inner.pop_back(); }

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

    std::shared_ptr<Value> find(std::string name) {
        for (auto s = inner.rbegin(); s != inner.rend(); s++) {
            auto iter = s->find(name);
            if (iter != s->end()) {
                return iter->second;
            }
        }

        return nullptr;
    }

  private:
    std::vector<std::map<std::string, std::shared_ptr<Value>>> inner;
    std::vector<std::map<std::string, std::vector<std::shared_ptr<Value>>>> array_param;
};

class CminusfBuilder : public ASTVisitor {
  public:
    CminusfBuilder() {
        module = std::unique_ptr<Module>(new Module("Cminus code"));
        builder = std::make_unique<IRBuilder>(nullptr, module.get());
        auto TyVoid = Type::get_void_type(module.get());
        auto TyInt32 = Type::get_int32_type(module.get());
        auto TyFloat = Type::get_float_type(module.get());

        auto input_type = FunctionType::get(TyInt32, {});
        auto input_fun = Function::create(input_type, "input", module.get());

        std::vector<Type *> output_params;
        output_params.push_back(TyInt32);
        auto output_type = FunctionType::get(TyVoid, output_params);
        auto output_fun = Function::create(output_type, "output", module.get());

        std::vector<Type *> output_float_params;
        output_float_params.push_back(TyFloat);
        auto output_float_type = FunctionType::get(TyVoid, output_float_params);
        auto output_float_fun = Function::create(output_float_type, "outputFloat", module.get());

        auto neg_idx_except_type = FunctionType::get(TyVoid, {});
        auto neg_idx_except_fun = Function::create(neg_idx_except_type, "neg_idx_except", module.get());

        scope.enter();
        scope.push("input", input_fun);
        scope.push("output", output_fun);
        scope.push("outputFloat", output_float_fun);
        scope.push("neg_idx_except", neg_idx_except_fun);
    }

   Module *getModule() { return module.get(); }

  private:
    virtual void visit(ASTProgram &) override final;
    virtual void visit(ASTNum &) override final;
    virtual void visit(ASTVarDeclaration &) override final;
    virtual void visit(ASTFunDeclaration &) override final;
    virtual void visit(ASTParam &) override final;
    virtual void visit(ASTCompoundStmt &) override final;
    virtual void visit(ASTExpressionStmt &) override final;
    virtual void visit(ASTSelectionStmt &) override final;
    virtual void visit(ASTIterationStmt &) override final;
    virtual void visit(ASTReturnStmt &) override final;
    virtual void visit(ASTAssignExpression &) override final;
    virtual void visit(ASTSimpleExpression &) override final;
    virtual void visit(ASTAdditiveExpression &) override final;
    virtual void visit(ASTVar &) override final;
    virtual void visit(ASTTerm &) override final;
    virtual void visit(ASTCall &) override final;

    std::unique_ptr<IRBuilder> builder;
    Scope scope;
    std::unique_ptr<Module> module;
};
#endif
