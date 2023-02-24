#include "AST.hpp"

#include <iostream>
#include <map>
#include <string>

#define _SYNTAX_TREE_NODE_ERROR_(REASON)                                                                               \
    std::cerr << "Abort due to node cast error:\n" << REASON << std::endl, std::abort();
void ASTCompUnit::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTConstDecl::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTConstDef::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTConstInitSingle::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTConstInitList::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTVarDecl::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTVarDef::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTInitSingle::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTInitList::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTFuncDef::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTFuncFParam::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTBlock::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTBlockItem::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTAssignStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTExpStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTSelectionStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTWhileStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTBreakStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTContinueStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTReturnStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTForStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTOpUnaryExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTAddExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTMulExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTCall::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTLVal::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTNumber::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTLOrExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTLAndExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTEqExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTRelExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTConstExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }

inline std::string _type_name(SysYType t) {
    if (t == TYPE_INT)
        return "int";
    else if (t == TYPE_FLOAT)
        return "float";
    else
        return "void";
}

std::map<RelOp, std::string> _rel_op{{OP_LE, "<="}, {OP_LT, "<"}, {OP_GT, ">"}, {OP_GE, ">="}};
std::map<AddOp, std::string> _add_op{{OP_PLUS, "+"}, {OP_MINUS, "-"}};
std::map<MulOp, std::string> _mul_op{{OP_MUL, "*"}, {OP_DIV, "/"}, {OP_MOD, "%"}};
std::map<UnaryOp, std::string> _unary_op{{OP_POS, "+"}, {OP_NEG, "-"}, {OP_NOT, "!"}};
std::map<EqOp, std::string> _eq_op{{OP_EQ, "=="}, {OP_NEQ, "!="}};
#define _INDENT_(n)                                                                                                    \
    { std::cout << std::string(2 * n, ' '); }

void ASTPrinter::visit(ASTCompUnit &node) {
    _INDENT_(depth);
    std::cout << "compile-unit" << std::endl;
    add_depth();
    for (auto decl : node.decl_func_list) {
        decl->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTConstDecl &node) {
    _INDENT_(depth);
    std::cout << "const-declaration: " << _type_name(node.type) << std::endl;
    add_depth();
    for (auto def : node.const_def_list) {
        def->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTConstDef &node) {
    _INDENT_(depth);
    std::cout << "const-def: " << node.id << std::endl;
    add_depth();
    for (auto array_decl : node.array_size) {
        array_decl->accept(*this);
    }
    if (node.const_init_val != nullptr) {
        node.const_init_val->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTConstInitSingle &node) {
    _INDENT_(depth);
    std::cout << "const-init-single" << std::endl;
    add_depth();
    node.exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTConstInitList &node) {
    _INDENT_(depth);
    std::cout << "const-init-list" << std::endl;
    add_depth();
    for (auto init_list : node.init_list) {
        init_list->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTVarDecl &node) {
    _INDENT_(depth);
    std::cout << "variable-declaration: " << _type_name(node.type) << std::endl;
    add_depth();
    for (auto def : node.var_def_list) {
        def->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTVarDef &node) {
    _INDENT_(depth);
    std::cout << "var-def: " << node.id << std::endl;
    add_depth();
    for (auto array_decl : node.array_size) {
        array_decl->accept(*this);
    }
    if (node.init_val != nullptr) {
        node.init_val->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTInitSingle &node) {
    _INDENT_(depth);
    std::cout << "car-init-single" << std::endl;
    add_depth();
    node.exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTInitList &node) {
    _INDENT_(depth);
    std::cout << "var-init-list" << std::endl;
    add_depth();
    for (auto init_list : node.init_list) {
        init_list->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTFuncDef &node) {
    _INDENT_(depth);
    std::cout << "func-def: " << node.id << std::endl;
    add_depth();
    for (auto par : node.param_list) {
        par->accept(*this);
    }
    node.block->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTFuncFParam &node) {
    _INDENT_(depth);
    std::cout << "param: " << _type_name(node.type) << " " << node.id << std::endl;
    add_depth();
    if (node.is_array) {
        _INDENT_(depth);
        std::cout << "[]" << std::endl;
        for (auto arr : node.array_exp_list) {
            arr->accept(*this);
        }
    }
    remove_depth();
}

void ASTPrinter::visit(ASTBlock &node) {
    _INDENT_(depth);
    std::cout << "block" << std::endl;
    add_depth();
    for (auto item : node.item_list) {
        item->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTBlockItem &node) {
    _INDENT_(depth);
    std::cout << "block-item" << std::endl;
    add_depth();
    if (node.const_decl != nullptr)
        node.const_decl->accept(*this);
    else if (node.var_decl != nullptr)
        node.var_decl->accept(*this);
    else if (node.stmt != nullptr)
        node.stmt->accept(*this);
    else
        _SYNTAX_TREE_NODE_ERROR_("block-item node error.")

    remove_depth();
}

void ASTPrinter::visit(ASTAssignStmt &node) {
    _INDENT_(depth);
    std::cout << "assign-stmt" << std::endl;
    add_depth();
    node.lval->accept(*this);
    node.exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTExpStmt &node) {
    _INDENT_(depth);
    std::cout << "expression-stmt" << std::endl;
    add_depth();
    if (node.exp != nullptr)
        node.exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTSelectionStmt &node) {
    _INDENT_(depth);
    std::cout << "selection-stmt" << std::endl;
    add_depth();
    node.cond->accept(*this);
    _INDENT_(depth);
    std::cout << "true-stmt" << std::endl;
    node.true_stmt->accept(*this);
    if (node.false_stmt) {
        _INDENT_(depth);
        std::cout << "false-stmt" << std::endl;
        node.false_stmt->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTWhileStmt &node) {
    _INDENT_(depth);
    std::cout << "while-stmt" << std::endl;
    add_depth();
    node.cond->accept(*this);
    node.stmt->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTBreakStmt &node) {
    _INDENT_(depth);
    std::cout << "break" << std::endl;
}

void ASTPrinter::visit(ASTContinueStmt &node) {
    _INDENT_(depth);
    std::cout << "continue" << std::endl;
}

void ASTPrinter::visit(ASTReturnStmt &node) {
    _INDENT_(depth);
    std::cout << "return" << std::endl;
    add_depth();
    if (node.exp != nullptr)
        node.exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTForStmt &node) {
    _INDENT_(depth);
    std::cout << "for-stmt" << std::endl;
    add_depth();
    node.var->accept(*this);
    node.init->accept(*this);
    node.cond->accept(*this);
    node.step->accept(*this);
    node.stmt->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTOpUnaryExp &node) {
    _INDENT_(depth);
    std::cout << "op-unary-exp: " << _unary_op[node.op] << std::endl;
    add_depth();
    node.unary_exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTAddExp &node) {
    _INDENT_(depth);
    std::cout << "add-exp: " << _add_op[node.op] << std::endl;
    add_depth();
    if (node.add_exp != nullptr)
        node.add_exp->accept(*this);
    node.mul_exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTMulExp &node) {
    _INDENT_(depth);
    std::cout << "mul-exp: " << _mul_op[node.op] << std::endl;
    add_depth();
    if (node.mul_exp != nullptr)
        node.mul_exp->accept(*this);
    node.unary_exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTCall &node) {
    _INDENT_(depth);
    std::cout << "call: " << node.id << std::endl;
    add_depth();
    for (auto param : node.params) {
        param->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTLVal &node) {
    _INDENT_(depth);
    std::cout << "L-val: " << node.id << std::endl;
    add_depth();
    for (auto index : node.index_list) {
        index->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTNumber &node) {
    _INDENT_(depth);
    if (node.type == TYPE_INT)
        std::cout << "number(int): " << node.int_val << std::endl;
    else if (node.type == TYPE_FLOAT)
        std::cout << "number(float): " << node.float_val << std::endl;
    else
        _SYNTAX_TREE_NODE_ERROR_("Unexpected number type.");
}

void ASTPrinter::visit(ASTLOrExp &node) {
    _INDENT_(depth);
    std::cout << "lor-exp" << std::endl;
    add_depth();
    if (node.lor_exp != nullptr)
        node.lor_exp->accept(*this);
    node.land_exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTLAndExp &node) {
    _INDENT_(depth);
    std::cout << "land-exp" << std::endl;
    add_depth();
    if (node.land_exp != nullptr)
        node.land_exp->accept(*this);
    node.eq_exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTEqExp &node) {
    _INDENT_(depth);
    std::cout << "eq-exp" << _eq_op[node.op] << std::endl;
    add_depth();
    if (node.eq_exp != nullptr)
        node.eq_exp->accept(*this);
    node.rel_exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTRelExp &node) {
    _INDENT_(depth);
    std::cout << "rel-exp" << _rel_op[node.op] << std::endl;
    add_depth();
    if (node.rel_exp != nullptr)
        node.rel_exp->accept(*this);
    node.add_exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTConstExp &node) {
    _INDENT_(depth);
    std::cout << "const-exp" << std::endl;
    add_depth();
    node.add_exp->accept(*this);
    remove_depth();
}
