#ifndef _AST_HPP_
#define _AST_HPP_

#include "SyntaxTree.hh"

#include <memory>
#include <string>
#include <vector>

// Difination of abstract syntax tree(AST).

enum SysYType { TYPE_INT, TYPE_FLOAT, TYPE_VOID };

enum RelOp {
    // <=
    OP_LE,
    // <
    OP_LT,
    // >
    OP_GT,
    // >=
    OP_GE
};

enum AddOp {
    // +
    OP_PLUS,
    // -
    OP_MINUS
};

enum MulOp {
    // *
    OP_MUL,
    // /
    OP_DIV,
    // %
    OP_MOD
};

enum UnaryOp {
    // +
    OP_POS,
    // -
    OP_NEG,
    // !
    OP_NOT
};

enum EqOp {
    // ==
    OP_EQ,
    // !=
    OP_NEQ
};

class AST;

struct ASTNode;
struct ASTCompUnit;
struct ASTDeclDef;
struct ASTConstDecl;
struct ASTConstDef;
struct ASTConstInitVal;
struct ASTVarDecl;
struct ASTVarDef;
struct ASTInitVal;
struct ASTFuncDef;
struct ASTFuncFParam;
struct ASTBlock;
struct ASTBlockItem;
struct ASTStmt;
struct ASTAssignStmt;
struct ASTExpStmt;
struct ASTSelectionStmt;
struct ASTIterationStmt;
struct ASTBreakStmt;
struct ASTContinueStmt;
struct ASTReturnStmt;
struct ASTForStmt;
struct ASTLVal;
struct ASTExp;
struct AddExp;
struct ASTPrimaryExp;
struct ASTAddExp;
struct ASTUnaryExp;
struct ASTCall;
struct ASTMulExp;
struct ASTCond;
struct ASTRelExp;
struct ASTEqExp;
struct ASTELAndxp;
struct ASTLOrExp;
struct ASTLAndExp;
struct ASTEqExp;
struct ASTRelExp;
struct ASTConstExp;

class ASTVisitor;

struct ASTNode {
    virtual void accept(ASTVisitor &) = 0;
    virtual ~ASTNode() {}
};

struct ASTCompUnit : ASTNode {
    virtual void accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTDeclDef>> decl_func_list;  // 可能为空
};

struct ASTDeclDef : ASTNode {
    SysYType type;
    // std::string id;
};

struct ASTConstDecl : ASTDeclDef {
    virtual void accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTConstDef>> const_def_list;
};

struct ASTConstDef : ASTNode {
    virtual void accept(ASTVisitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ASTConstExp>> array_size;  // 可能为空，此时该常量不是数组
    std::shared_ptr<ASTConstInitVal> const_init_val;       // 不能为空，常量必须初始化
};

struct ASTConstInitVal : ASTNode {};

struct ASTConstInitSingle : ASTConstInitVal {
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTConstExp> exp;
};

struct ASTConstInitList : ASTConstInitVal {
    virtual void accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTConstInitVal>> init_list;  // 可能为空，表示全部初始化为0
};

struct ASTVarDecl : ASTDeclDef {
    virtual void accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTVarDef>> var_def_list;
};

struct ASTVarDef : ASTNode {
    virtual void accept(ASTVisitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ASTConstExp>> array_size;  // 可能为空
    std::shared_ptr<ASTInitVal> init_val;                  // 可能为空
};

// struct ASTInitVal : ASTNode {
//     virtual void accept(ASTVisitor &) override final;
//     std::shared_ptr<ASTExp> exp;
//     std::vector<std::shared_ptr<ASTInitVal>> init_list;
// };

struct ASTInitVal : ASTNode {};

struct ASTInitSingle : ASTInitVal {
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> exp;
};

struct ASTInitList : ASTInitVal {
    virtual void accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTInitVal>> init_list;  // 可能为空，全部初始化为0
};

struct ASTFuncDef : ASTDeclDef {
    virtual void accept(ASTVisitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ASTFuncFParam>> param_list;  // 可能为空
    std::shared_ptr<ASTBlock> block;
};

struct ASTFuncFParam : ASTNode {
    virtual void accept(ASTVisitor &) override final;
    SysYType type;
    std::string id;
    bool is_array;
    std::vector<std::shared_ptr<ASTExp>> array_exp_list;  // 可能为空，即使is_array为true
};

struct ASTStmt : ASTNode {};

struct ASTBlock : ASTStmt {
    virtual void accept(ASTVisitor &) override final;
    std::vector<std::shared_ptr<ASTBlockItem>> item_list;  // 可能为空
};

struct ASTBlockItem : ASTNode {
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTConstDecl> const_decl;  // 只有一个非空
    std::shared_ptr<ASTVarDecl> var_decl;      // 只有一个非空
    std::shared_ptr<ASTStmt> stmt;             // 只有一个非空
};

struct ASTAssignStmt : ASTStmt {
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTLVal> lval;
    std::shared_ptr<ASTExp> exp;
};

struct ASTExpStmt : ASTStmt {
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> exp;  // 可能为空
};

struct ASTSelectionStmt : ASTStmt {
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTCond> cond;
    std::shared_ptr<ASTStmt> true_stmt;   // 不能为空
    std::shared_ptr<ASTStmt> false_stmt;  // 可能为空
};

struct ASTWhileStmt : ASTStmt {
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTCond> cond;
    std::shared_ptr<ASTStmt> stmt;
};

struct ASTBreakStmt : ASTStmt {
    virtual void accept(ASTVisitor &) override final;
};

struct ASTContinueStmt : ASTStmt {
    virtual void accept(ASTVisitor &) override final;
};

struct ASTReturnStmt : ASTStmt {
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTExp> exp;  // 可能为空
};

struct ASTForStmt : ASTStmt {
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTLVal> var;
    std::shared_ptr<ASTExp> init;
    std::shared_ptr<ASTCond> cond;
    std::shared_ptr<ASTExp> step;
    std::shared_ptr<ASTStmt> stmt;
};

struct ASTUnaryExp : ASTNode {};

struct ASTOpUnaryExp : ASTUnaryExp {
    virtual void accept(ASTVisitor &) override final;
    UnaryOp op;
    std::shared_ptr<ASTUnaryExp> unary_exp;
};

struct ASTPrimaryExp : ASTUnaryExp {};

struct ASTExp : ASTPrimaryExp {};

struct ASTAddExp : ASTExp {
    virtual void accept(ASTVisitor &) override final;
    AddOp op;
    std::shared_ptr<ASTMulExp> mul_exp;
    std::shared_ptr<ASTAddExp> add_exp;  // 可能为空
};

struct ASTMulExp : ASTNode {
    virtual void accept(ASTVisitor &) override final;
    MulOp op;
    std::shared_ptr<ASTUnaryExp> unary_exp;
    std::shared_ptr<ASTMulExp> mul_exp;  // 可能为空
};

struct ASTCall : ASTUnaryExp {
    virtual void accept(ASTVisitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ASTExp>> params;  // 可能为空
};

struct ASTLVal : ASTPrimaryExp {
    virtual void accept(ASTVisitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ASTExp>> index_list;  // 可能为空
};

struct ASTNumber : ASTPrimaryExp {
    virtual void accept(ASTVisitor &) override final;
    SysYType type;
    union {
        int int_val;
        float float_val;
    };
};

struct ASTCond : ASTNode {};

struct ASTLOrExp : ASTCond {
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTLAndExp> land_exp;
    std::shared_ptr<ASTLOrExp> lor_exp;  // 可能为空
};

struct ASTLAndExp : ASTNode {
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTLAndExp> land_exp;  // 可能为空
    std::shared_ptr<ASTEqExp> eq_exp;
};

struct ASTEqExp : ASTNode {
    virtual void accept(ASTVisitor &) override final;
    EqOp op;
    std::shared_ptr<ASTEqExp> eq_exp;
    std::shared_ptr<ASTRelExp> rel_exp;  // 可能为空
};

struct ASTRelExp : ASTNode {
    virtual void accept(ASTVisitor &) override final;
    RelOp op;
    std::shared_ptr<ASTRelExp> rel_exp;  // 可能为空
    std::shared_ptr<ASTAddExp> add_exp;
};

struct ASTConstExp : ASTNode {
    virtual void accept(ASTVisitor &) override final;
    std::shared_ptr<ASTAddExp> add_exp;
};

class AST {
  public:
    AST() = delete;
    AST(SyntaxTree *syntax_tree);
    AST(ASTCompUnit *_root) { root = std::shared_ptr<ASTCompUnit>(_root); }
    AST(AST &&tree) {
        root = tree.root;
        tree.root = nullptr;
    }
    ASTCompUnit *get_root() { return root.get(); }
    void run_visitor(ASTVisitor &visitor);

  private:
    std::shared_ptr<ASTCompUnit> root = nullptr;
};

class ASTVisitor {
  public:
    virtual void visit(ASTCompUnit &) = 0;
    virtual void visit(ASTConstDecl &) = 0;
    virtual void visit(ASTConstDef &) = 0;
    virtual void visit(ASTConstInitSingle &) = 0;
    virtual void visit(ASTConstInitList &) = 0;
    virtual void visit(ASTVarDecl &) = 0;
    virtual void visit(ASTVarDef &) = 0;
    virtual void visit(ASTInitSingle &) = 0;
    virtual void visit(ASTInitList &) = 0;
    virtual void visit(ASTFuncDef &) = 0;
    virtual void visit(ASTFuncFParam &) = 0;
    virtual void visit(ASTBlock &) = 0;
    virtual void visit(ASTBlockItem &) = 0;
    virtual void visit(ASTAssignStmt &) = 0;
    virtual void visit(ASTExpStmt &) = 0;
    virtual void visit(ASTSelectionStmt &) = 0;
    virtual void visit(ASTWhileStmt &) = 0;
    virtual void visit(ASTBreakStmt &) = 0;
    virtual void visit(ASTContinueStmt &) = 0;
    virtual void visit(ASTReturnStmt &) = 0;
    virtual void visit(ASTForStmt &) = 0;
    virtual void visit(ASTOpUnaryExp &) = 0;
    virtual void visit(ASTAddExp &) = 0;
    virtual void visit(ASTMulExp &) = 0;
    virtual void visit(ASTCall &) = 0;
    virtual void visit(ASTLVal &) = 0;
    virtual void visit(ASTNumber &) = 0;
    virtual void visit(ASTLOrExp &) = 0;
    virtual void visit(ASTLAndExp &) = 0;
    virtual void visit(ASTEqExp &) = 0;
    virtual void visit(ASTRelExp &) = 0;
    virtual void visit(ASTConstExp &) = 0;
};

class ASTPrinter : public ASTVisitor {
  public:
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
    void add_depth() { depth += 2; }
    void remove_depth() { depth -= 2; }

  private:
    int depth = 0;
};
#endif
