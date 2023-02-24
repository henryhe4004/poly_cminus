#pragma once
#include "AST.hpp"
#include "Module.h"
#include "SYSYCBuilder.h"
class Polyhedral;
class PolyStmt;
// mostly copied from our original builder, with small (mostly in Functions) modifications
class PolyBuilder : public ASTVisitor {
  friend Polyhedral; // messy friendship
  public:
    PolyBuilder(Module *m_, Polyhedral *poly_m) : module(m_), poly_m(poly_m), builder(new IRBuilder(nullptr, module)) {
        scope.enter();
    }
    void set_insert_point(std::shared_ptr<BasicBlock> bb);
    void replace_iv(ASTCall &call_node);
    void copy_stmt_recur(Instruction*);
  
    BasicBlock* get_loop_before_blocks() { return this->loop_before; }
    BasicBlock* get_loop_exit_blocks() { return this->loop_exit; }
    void add_loop_before_block(BasicBlock* bb) { this->loop_before = bb; }
    void add_loop_exit_block(BasicBlock* bb) { this->loop_exit=bb; }

    // the rest is (basically) of no use
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
    enum class bin_op { add, sub, mul, div, mod };

    std::shared_ptr<Constant> build_up_initializer(std::vector<std::shared_ptr<Constant>> line);
    std::shared_ptr<Value> calculate(std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs, bin_op op);
    std::shared_ptr<Value> convert(std::shared_ptr<Value> now, Type *to);

  private:
    // as in isl context, the constant [N]s that are invariants throughout the loops
    std::map<std::string, std::shared_ptr<Value>> context;
    std::unordered_map<std::string ,std::shared_ptr<PolyStmt>> name2stmt;
    Module *module;
    Polyhedral *poly_m;
    Scope scope;
    std::unique_ptr<IRBuilder> builder;
    std::list<std::shared_ptr<AllocaInst>> alloca_instructions;
    BasicBlock* loop_before;
    BasicBlock* loop_exit;
};
