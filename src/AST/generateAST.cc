#include "AST.hpp"
#include "SyntaxTree.hh"

#include <algorithm>
#include <assert.h>
#include <cstdio>
#include <map>
#include <regex>
#include <sstream>

using std::make_shared;
using std::map;
using std::shared_ptr;
using std::string;
using std::vector;

SysYType cTypeToASTYType(string type, bool isFunc = false) {
    if (type == "int")
        return SysYType::TYPE_INT;
    if (type == "float")
        return SysYType::TYPE_FLOAT;
    if (type == "void" and isFunc)
        return SysYType::TYPE_VOID;
    std::cerr << "Error: void type is not allowed" << std::endl;
    std::abort();
}

map<string, RelOp> cRelOpToASTRelOp{{"<=", RelOp::OP_LE},
                                    {"<", RelOp::OP_LT},
                                    {">", RelOp::OP_GT},
                                    {">=", RelOp::OP_GE}};

map<string, AddOp> cAddOpToASTAddOp{{"+", AddOp::OP_PLUS}, {"-", AddOp::OP_MINUS}};
map<string, MulOp> cMulOpToASTMulOp{{"*", MulOp::OP_MUL}, {"/", MulOp::OP_DIV}, {"%", MulOp::OP_MOD}};
map<string, UnaryOp> cUnaryOpToASTUnaryOp{{"+", UnaryOp::OP_POS}, {"-", UnaryOp::OP_NEG}, {"!", UnaryOp::OP_NOT}};
map<string, EqOp> cEqOpToASTEqOp{{"==", EqOp::OP_EQ}, {"!=", EqOp::OP_NEQ}};

float get_float_constant_value(std::string o) {
    std::stringstream value_stream(o);
    float value = 0;
    // std::regex decimal_floating_constant(
    //     R"axaz(([0-9]*\.[0-9]+|[0-9]+\.)([eE][+-]?[0-9]+)?|[0-9]+([eE][+-]?[0-9]+))axaz");
    // std::regex hexadecimal_floating_constant(
    //     R"axaz(0[xX]([0-9a-fA-F]*\.[0-9a-fA-F]+|[0-9a-fA-F]+\.?)[pP][+-]?[0-9]+)axaz");
    if (o.find('x') == string::npos && o.find('X') == string::npos) {
        value_stream >> value;
        return value;
    } else {
        // value_stream >> std::hexfloat >> value;
        // return value;
        sscanf(o.c_str(), "%a", &value);
        return value;
    }
    return 0;
}

shared_ptr<ASTCompUnit> genASTCompUnit(SyntaxTreeNode *compUnit);
shared_ptr<ASTDeclDef> genASTDeclDef(SyntaxTreeNode *declOrFuncDef);
shared_ptr<ASTStmt> genASTStmt(SyntaxTreeNode *stmt);
shared_ptr<ASTAddExp> genASTAddExp(SyntaxTreeNode *addExp);
shared_ptr<ASTMulExp> genASTMulExp(SyntaxTreeNode *mulExp);
shared_ptr<ASTUnaryExp> genASTUnaryExp(SyntaxTreeNode *unaryExp);
shared_ptr<ASTLOrExp> genASTLOrExp(SyntaxTreeNode *lOrExp);
shared_ptr<ASTLAndExp> genASTLAndExp(SyntaxTreeNode *lAndExp);
shared_ptr<ASTEqExp> genASTEqExp(SyntaxTreeNode *eqExp);
shared_ptr<ASTRelExp> genASTRelExp(SyntaxTreeNode *relExp);
shared_ptr<ASTPrimaryExp> genASTPrimaryExp(SyntaxTreeNode *primaryExp);
shared_ptr<ASTLVal> genASTLVal(SyntaxTreeNode *lVal);
shared_ptr<ASTConstDecl> genASTConstDecl(SyntaxTreeNode *decl);
shared_ptr<ASTConstDef> genASTConstDef(SyntaxTreeNode *constDef);
shared_ptr<ASTConstInitVal> genASTConstInitVal(SyntaxTreeNode *constInitVal);
shared_ptr<ASTVarDecl> genASTVarDecl(SyntaxTreeNode *decl);
shared_ptr<ASTVarDef> genASTVarDef(SyntaxTreeNode *varDef);
shared_ptr<ASTInitVal> genASTInitVal(SyntaxTreeNode *initVal);
shared_ptr<ASTFuncFParam> genASTFuncFParam(SyntaxTreeNode *funcFParam);
shared_ptr<ASTBlock> genASTBlock(SyntaxTreeNode *block);
shared_ptr<ASTBlockItem> genASTBlockItem(SyntaxTreeNode *blockItem);
shared_ptr<ASTDeclDef> dealASTDecl(SyntaxTreeNode *decl);
shared_ptr<ASTExp> dealASTExpOptional(SyntaxTreeNode *expOptional);
shared_ptr<ASTExp> dealASTExp(SyntaxTreeNode *exp);
shared_ptr<ASTConstExp> dealASTConstExp(SyntaxTreeNode *constExp);
shared_ptr<ASTLOrExp> dealASTCond(SyntaxTreeNode *cond);
vector<SyntaxTreeNode *> getListFromGroup(SyntaxTreeNode *group, int targetIndex);

AST::AST(SyntaxTree *syntax_tree) {
    if (syntax_tree == nullptr) {
        std::cerr << "empty input tree!" << std::endl;
        std::abort();
    }
    root = genASTCompUnit(syntax_tree->root);
    delete_all_children_node(syntax_tree->root);
}

// CompUnit        : CompUnit_Optional Decl_or_FuncDef
// CompUnit_Optional : CompUnit |
shared_ptr<ASTCompUnit> genASTCompUnit(SyntaxTreeNode *compUnit) {
    assert(compUnit->name == "CompUnit");
    vector<SyntaxTreeNode *> allDeclOrFuncdef{compUnit->children[1]};

    auto compUnitOptional = compUnit->children[0];
    while (compUnitOptional->children.size() > 0) {
        compUnit = compUnitOptional->children[0];
        allDeclOrFuncdef.push_back(compUnit->children[1]);
        compUnitOptional = compUnit->children[0];
    }
    std::reverse(allDeclOrFuncdef.begin(), allDeclOrFuncdef.end());
    auto result = make_shared<ASTCompUnit>();
    auto allASTDeclOrFunc = vector<shared_ptr<ASTDeclDef>>();
    for (auto declOrFunc : allDeclOrFuncdef)
        result->decl_func_list.push_back(genASTDeclDef(declOrFunc));
    return result;
}

// Decl_or_FuncDef : Decl | FuncDef
// FuncDef : BType Ident LPAREN FuncFParams_Optional RPAREN Block
// FuncFParams_Optional : FuncFParams |
// FuncFParams : FuncFParam COMMA_FuncFParam_Group
// COMMA_FuncFParam_Group : COMMA_FuncFParam_Group COMMA FuncFParam |
shared_ptr<ASTDeclDef> genASTDeclDef(SyntaxTreeNode *declOrFuncDef) {
    assert(declOrFuncDef->name == "Decl_or_FuncDef");
    auto children0Name = declOrFuncDef->children[0]->name;

    if (children0Name == "Decl") {
        auto result = dealASTDecl(declOrFuncDef->children[0]);
        return result;
    } else if (children0Name == "FuncDef") {
        auto funcDef = declOrFuncDef->children[0];
        auto funcType = funcDef->children[0]->children[0];
        auto ident = funcDef->children[1];
        auto funcFParams_Optional = funcDef->children[3];
        auto block = funcDef->children[5];

        vector<SyntaxTreeNode *> allFuncFParam;
        if (funcFParams_Optional->children.size() > 0) {
            auto funcFParams = funcFParams_Optional->children[0];
            auto commaFParamGroup = funcFParams->children[1];
            auto funcFParam = funcFParams->children[0];
            allFuncFParam = getListFromGroup(commaFParamGroup, 2);
            allFuncFParam.insert(allFuncFParam.begin(), funcFParam);
        }

        auto result = make_shared<ASTFuncDef>();
        result->block = genASTBlock(block);
        result->id = ident->name;
        result->type = cTypeToASTYType(funcType->name, true);
        auto &allASTFuncFParam = result->param_list;
        for (auto funcFParam : allFuncFParam)
            allASTFuncFParam.push_back(genASTFuncFParam(funcFParam));
        return result;
    }
    std::abort();
}

// Stmt : LVal ASSIGN Exp SEMICOLON | Exp_Optional SEMICOLON | Block
//      | IF LPAREN Cond RPAREN Stmt ELSE_Stmt_Optional | WHILE LPAREN Cond RPAREN Stmt
//      | BREAK SEMICOLON | CONTINUE SEMICOLON | RETURN Exp_Optional SEMICOLON
//      | FOR LPAREN BType LVal ASSIGN Exp SEMICOLON Cond SEMICOLON LVal ADD_ASSIGN Exp RPAREN Stmt;
shared_ptr<ASTStmt> genASTStmt(SyntaxTreeNode *stmt) {
    assert(stmt->name == "Stmt");
    auto children0Name = stmt->children[0]->name;
    if (children0Name == "if") {
        auto cond = stmt->children[2];
        auto ifStmt = stmt->children[4];
        auto elseStmtOptional = stmt->children[5];
        shared_ptr<ASTStmt> astElseStmt;
        if (elseStmtOptional->children.size() > 0) {
            auto elseStmt = elseStmtOptional->children[1];
            astElseStmt = genASTStmt(elseStmt);
        } else
            astElseStmt = nullptr;

        auto astIfStmt = genASTStmt(ifStmt);
        auto astCond = dealASTCond(cond);
        auto result = make_shared<ASTSelectionStmt>();
        result->cond = astCond;
        result->true_stmt = astIfStmt;
        result->false_stmt = astElseStmt;
        return result;
    } else if (children0Name == "while") {
        auto cond = stmt->children[2];
        auto insideStmt = stmt->children[4];
        auto astWhileStmt = genASTStmt(insideStmt);
        auto astCond = dealASTCond(cond);
        auto result = make_shared<ASTWhileStmt>();
        result->cond = astCond;
        result->stmt = astWhileStmt;
        return result;
    } else if (children0Name == "break") {
        auto result = make_shared<ASTBreakStmt>();
        return result;
    } else if (children0Name == "continue") {
        auto result = make_shared<ASTContinueStmt>();
        return result;
    } else if (children0Name == "return") {
        auto expOptional = stmt->children[1];
        auto astExpOptional = dealASTExpOptional(expOptional);
        auto result = make_shared<ASTReturnStmt>();
        result->exp = astExpOptional;
        return result;
    } else if (children0Name == "Exp_Optional") {
        auto astExpOptional = dealASTExpOptional(stmt->children[0]);
        auto result = make_shared<ASTExpStmt>();
        result->exp = astExpOptional;
        return result;
    } else if (children0Name == "Block") {
        auto result = genASTBlock(stmt->children[0]);
        return result;
    } else if (children0Name == "LVal") {
        auto lVal = stmt->children[0];
        auto addExp = stmt->children[2]->children[0];
        auto astLVal = genASTLVal(lVal);
        auto astAddExp = genASTAddExp(addExp);
        auto result = make_shared<ASTAssignStmt>();
        result->lval = astLVal;
        result->exp = astAddExp;
        return result;
    } else if (children0Name == "for") {
        // FOR LVal BType Ident ASSIGN Exp SEMICOLON Cond SEMICOLON LVal ADD_ASSIGN Exp RPAREN Stmt;
        auto var = stmt->children[3];
        auto init = stmt->children[5];
        auto cond = stmt->children[7];
        auto step = stmt->children[11];
        auto insideStmt = stmt->children[13];
        auto astVar = genASTLVal(var);
        auto astInit = dealASTExp(init);
        auto astCond = dealASTCond(cond);
        auto astStep = dealASTExp(step);
        auto astInsideStmt = genASTStmt(insideStmt);
        auto result = make_shared<ASTForStmt>();
        result->var = astVar;
        result->init = astInit;
        result->cond = astCond;
        result->step = astStep;
        result->stmt = astInsideStmt;
        return result;
    }
    std::abort();
}

// AddExp : MulExp | AddExp AddOp MulExp
shared_ptr<ASTAddExp> genASTAddExp(SyntaxTreeNode *addExp) {
    assert(addExp->name == "AddExp");
    auto result = make_shared<ASTAddExp>();
    if (addExp->children.size() == 1) {
        auto mulExp = addExp->children[0];
        auto astMulExp = genASTMulExp(mulExp);
        result->add_exp = nullptr;
        result->mul_exp = astMulExp;
        // result->op = none
    } else {
        auto subAddExp = addExp->children[0];
        auto op = addExp->children[1]->children[0];
        auto mulExp = addExp->children[2];
        auto astAddExp = genASTAddExp(subAddExp);
        auto astMulExp = genASTMulExp(mulExp);
        result->add_exp = astAddExp;
        result->mul_exp = astMulExp;
        result->op = cAddOpToASTAddOp.at(op->name);
    }
    return result;
}

// MulExp : UnaryExp | MulExp MulOp UnaryExp
shared_ptr<ASTMulExp> genASTMulExp(SyntaxTreeNode *mulExp) {
    assert(mulExp->name == "MulExp");
    auto result = make_shared<ASTMulExp>();
    if (mulExp->children.size() == 1) {
        auto unaryExp = mulExp->children[0];
        auto astUnaryExp = genASTUnaryExp(unaryExp);
        result->mul_exp = nullptr;
        result->unary_exp = astUnaryExp;
        // result->op = none
    } else {
        auto subMulExp = mulExp->children[0];
        auto op = mulExp->children[1]->children[0];
        auto unaryExp = mulExp->children[2];
        auto astMulExp = genASTMulExp(subMulExp);
        auto astUnaryExp = genASTUnaryExp(unaryExp);
        result->mul_exp = astMulExp;
        result->unary_exp = astUnaryExp;
        result->op = cMulOpToASTMulOp.at(op->name);
    }
    return result;
}

// UnaryExp : PrimaryExp | Ident LPAREN FuncRParams_Optional RPAREN | UnaryOp UnaryExp
// FuncRParams_Optional : FuncRParams |
// FuncRParams : Exp COMMA_Exp_Group
// COMMA_Exp_Group : COMMA_Exp_Group COMMA Exp |
shared_ptr<ASTUnaryExp> genASTUnaryExp(SyntaxTreeNode *unaryExp) {
    assert(unaryExp->name == "UnaryExp");
    if (unaryExp->children.size() == 1) {
        auto primaryExp = unaryExp->children[0];
        auto result = genASTPrimaryExp(primaryExp);
        return result;
    } else if (unaryExp->children.size() == 4) {
        auto ident = unaryExp->children[0];
        auto funcRParamsOptional = unaryExp->children[2];

        vector<SyntaxTreeNode *> allExp;
        if (funcRParamsOptional->children.size() > 0) {
            auto funcRParams = funcRParamsOptional->children[0];
            auto exp = funcRParams->children[0];
            auto commaFParamGroup = funcRParams->children[1];
            allExp = getListFromGroup(commaFParamGroup, 2);
            allExp.insert(allExp.begin(), exp);
        }
        auto result = make_shared<ASTCall>();
        result->id = ident->name;
        auto &allASTExp = result->params;
        for (auto exp : allExp)
            allASTExp.push_back(dealASTExp(exp));
        return result;
    } else {
        auto op = unaryExp->children[0];
        auto subUnaryExp = unaryExp->children[1];
        auto astUnaryExp = genASTUnaryExp(subUnaryExp);
        auto result = make_shared<ASTOpUnaryExp>();
        result->unary_exp = astUnaryExp;
        result->op = cUnaryOpToASTUnaryOp.at(op->children[0]->name);
        return result;
    }
}

// LOrExp : LAndExp | LOrExp OR LAndExp
shared_ptr<ASTLOrExp> genASTLOrExp(SyntaxTreeNode *lOrExp) {
    assert(lOrExp->name == "LOrExp");
    auto result = make_shared<ASTLOrExp>();
    if (lOrExp->children.size() == 1) {
        auto lAndExp = lOrExp->children[0];
        auto astLAndExp = genASTLAndExp(lAndExp);
        result->lor_exp = nullptr;
        result->land_exp = astLAndExp;
    } else {
        auto subLOrExp = lOrExp->children[0];
        // auto op = lOrExp->children[1]->children[0];
        auto lAndExp = lOrExp->children[2];
        auto astLOrExp = genASTLOrExp(subLOrExp);
        auto astLAndExp = genASTLAndExp(lAndExp);
        result->lor_exp = astLOrExp;
        result->land_exp = astLAndExp;
    }
    return result;
}

// LAndExp : EqExp | LAndExp AND EqExp
shared_ptr<ASTLAndExp> genASTLAndExp(SyntaxTreeNode *lAndExp) {
    assert(lAndExp->name == "LAndExp");
    auto result = make_shared<ASTLAndExp>();
    if (lAndExp->children.size() == 1) {
        auto eqExp = lAndExp->children[0];
        auto astEqExp = genASTEqExp(eqExp);
        result->land_exp = nullptr;
        result->eq_exp = astEqExp;
    } else {
        auto subLAndExp = lAndExp->children[0];
        // auto op = lAndExp->children[1]->children[0];
        auto eqExp = lAndExp->children[2];
        auto astLAndExp = genASTLAndExp(subLAndExp);
        auto astEqExp = genASTEqExp(eqExp);
        result->land_exp = astLAndExp;
        result->eq_exp = astEqExp;
    }
    return result;
}

// EqExp : RelExp | EqExp EqOp RelExp
shared_ptr<ASTEqExp> genASTEqExp(SyntaxTreeNode *eqExp) {
    assert(eqExp->name == "EqExp");
    auto result = make_shared<ASTEqExp>();
    if (eqExp->children.size() == 1) {
        auto relExp = eqExp->children[0];
        auto astRelExp = genASTRelExp(relExp);
        result->eq_exp = nullptr;
        result->rel_exp = astRelExp;
        // result->op =
    } else {
        auto subEqExp = eqExp->children[0];
        auto op = eqExp->children[1]->children[0];
        auto relExp = eqExp->children[2];
        auto astEqExp = genASTEqExp(subEqExp);
        auto astRelExp = genASTRelExp(relExp);
        result->eq_exp = astEqExp;
        result->rel_exp = astRelExp;
        result->op = cEqOpToASTEqOp.at(op->name);
    }
    return result;
}

// RelExp : AddExp | RelExp RelOp AddExp
shared_ptr<ASTRelExp> genASTRelExp(SyntaxTreeNode *relExp) {
    assert(relExp->name == "RelExp");
    auto result = make_shared<ASTRelExp>();
    if (relExp->children.size() == 1) {
        auto addExp = relExp->children[0];
        auto astAddExp = genASTAddExp(addExp);
        result->rel_exp = nullptr;
        result->add_exp = astAddExp;
    } else {
        auto subRelExp = relExp->children[0];
        auto op = relExp->children[1]->children[0];
        auto addExp = relExp->children[2];
        auto astRelExp = genASTRelExp(subRelExp);
        auto astAddExp = genASTAddExp(addExp);
        result->rel_exp = astRelExp;
        result->add_exp = astAddExp;
        result->op = cRelOpToASTRelOp.at(op->name);
    }
    return result;
}

// PrimaryExp : LPAREN Exp RPAREN | LVal | Number
// Number     : IntConst | floatConst
shared_ptr<ASTPrimaryExp> genASTPrimaryExp(SyntaxTreeNode *primaryExp) {
    assert(primaryExp->name == "PrimaryExp");
    if (primaryExp->children.size() == 3) {
        auto exp = primaryExp->children[1];
        auto astExp = dealASTExp(exp);
        return astExp;
    } else if (primaryExp->children[0]->name == "LVal") {
        auto lVal = primaryExp->children[0];
        auto astLVal = genASTLVal(lVal);
        return astLVal;
    } else if (primaryExp->children[0]->name == "Number") {
        auto number = primaryExp->children[0];
        auto numberValue = number->children[0]->name;
        auto result = make_shared<ASTNumber>();
        // std::regex floating_constant(
        //     R"axaz(0[xX]([0-9a-fA-F]*\.[0-9a-fA-F]+|[0-9a-fA-F]+\.?)[pP][+-]?[0-9]+|([0-9]*\.[0-9]+|[0-9]+\.)([eE][+-]?[0-9]+)?|[0-9]+([eE][+-]?[0-9]+))axaz");
        // 有小数点，或有[pP]，或没有[xX]（不是十六进制整数）却有[eE]
        if (numberValue.find('.') != string::npos || numberValue.find('p') != string::npos ||
            numberValue.find('P') != string::npos ||
            (numberValue.find('x') == string::npos && numberValue.find('X') == string::npos &&
             (numberValue.find('e') != string::npos || numberValue.find('E') != string::npos))) {
            result->type = SysYType::TYPE_FLOAT;
            result->float_val = get_float_constant_value(numberValue);
        } else {
            // 输入 -2147483648 时，numberValue = "2147483648"，直接转换会报错。所以取反，转换，再取反
            // -2147483648 取反的结果仍然是 -2147483648
            result->type = SysYType::TYPE_INT;
            result->int_val = -std::stoi("-" + numberValue, nullptr, 0);  // 0 表示自动推断进制
        }
        return result;
    }
    std::abort();
}

// LVal                        : Ident LBRACKET_Exp_RBRACKET_Group
// LBRACKET_Exp_RBRACKET_Group : LBRACKET_Exp_RBRACKET_Group LBRACKET Exp RBRACKET |
shared_ptr<ASTLVal> genASTLVal(SyntaxTreeNode *lVal) {
    assert(lVal->name == "LVal");
    auto ident = lVal->children[0];
    auto lBracketExpRBracketGroup = lVal->children[1];
    auto allExp = getListFromGroup(lBracketExpRBracketGroup, 2);
    auto allAstExp = vector<shared_ptr<ASTExp>>();
    for (auto exp : allExp)
        allAstExp.push_back(dealASTExp(exp));
    auto result = make_shared<ASTLVal>();
    result->id = ident->name;
    result->index_list = allAstExp;
    return result;
}

// ConstDecl            : CONST BType ConstDef COMMA_ConstDef_Group SEMICOLON
// COMMA_ConstDef_Group : COMMA_ConstDef_Group COMMA ConstDef |
shared_ptr<ASTConstDecl> genASTConstDecl(SyntaxTreeNode *decl) {
    assert(decl->name == "ConstDecl");
    auto bType = decl->children[1]->children[0];
    auto constDef = decl->children[2];
    auto commaConstDefGroup = decl->children[3];
    auto allConstDef = getListFromGroup(commaConstDefGroup, 2);
    allConstDef.insert(allConstDef.begin(), constDef);

    auto allASTConstDef = vector<shared_ptr<ASTConstDef>>();
    for (auto def : allConstDef)
        allASTConstDef.push_back(genASTConstDef(def));
    auto result = make_shared<ASTConstDecl>();
    result->type = cTypeToASTYType(bType->name);
    result->const_def_list = allASTConstDef;
    return result;
}

// ConstDef                         : Ident LBRACKET_ConstExp_RBRACKET_Group ASSIGN ConstInitVal
// LBRACKET_ConstExp_RBRACKET_Group : LBRACKET_ConstExp_RBRACKET_Group LBRACKET ConstExp RBRACKET |
shared_ptr<ASTConstDef> genASTConstDef(SyntaxTreeNode *constDef) {
    assert(constDef->name == "ConstDef");
    auto ident = constDef->children[0];
    auto constInitVal = constDef->children[3];
    auto lBracketConstExpRBracketGroup = constDef->children[1];
    auto allConstExp = getListFromGroup(lBracketConstExpRBracketGroup, 2);
    auto allAstConstExp = vector<shared_ptr<ASTConstExp>>();
    for (auto exp : allConstExp)
        allAstConstExp.push_back(dealASTConstExp(exp));
    auto astConstInitVal = genASTConstInitVal(constInitVal);
    auto result = make_shared<ASTConstDef>();
    result->id = ident->name;
    result->array_size = allAstConstExp;
    result->const_init_val = astConstInitVal;
    return result;
}

// ConstInitVal : ConstExp | LBRACE ConstInitVal_COMMA_ConstInitVal_Group_Optional RBRACE
// ConstInitVal_COMMA_ConstInitVal_Group_Optional : ConstInitVal COMMA_ConstInitVal_Group |
// COMMA_ConstInitVal_Group : COMMA_ConstInitVal_Group COMMA ConstInitVal |
shared_ptr<ASTConstInitVal> genASTConstInitVal(SyntaxTreeNode *constInitVal) {
    assert(constInitVal->name == "ConstInitVal");
    if (constInitVal->children.size() == 1) {
        auto constExp = constInitVal->children[0];
        auto astConstExp = dealASTConstExp(constExp);
        auto result = make_shared<ASTConstInitSingle>();
        result->exp = astConstExp;
        return result;
    } else {
        auto result = make_shared<ASTConstInitList>();
        auto constInitValCommaConstInitValGroupOptional = constInitVal->children[1];
        if (constInitValCommaConstInitValGroupOptional->children.size() > 0) {
            auto constInitVal1 = constInitValCommaConstInitValGroupOptional->children[0];
            auto commaConstInitValGroup = constInitValCommaConstInitValGroupOptional->children[1];
            auto allConstInitVal = getListFromGroup(commaConstInitValGroup, 2);
            allConstInitVal.insert(allConstInitVal.begin(), constInitVal1);

            auto &allAstConstInitVal = result->init_list;
            for (auto initVal : allConstInitVal)
                allAstConstInitVal.push_back(genASTConstInitVal(initVal));
        }
        return result;
    }
}

// VarDecl            : BType VarDef COMMA_VarDef_Group SEMICOLON
// COMMA_VarDef_Group : COMMA_VarDef_Group COMMA VarDef |
shared_ptr<ASTVarDecl> genASTVarDecl(SyntaxTreeNode *decl) {
    assert(decl->name == "VarDecl");
    auto declType = decl->children[0]->children[0];
    auto varDef = decl->children[1];
    auto commaVarDefGroup = decl->children[2];

    auto allVarDef = getListFromGroup(commaVarDefGroup, 2);
    allVarDef.insert(allVarDef.begin(), varDef);
    auto allASTVarDef = vector<shared_ptr<ASTVarDef>>();
    for (auto varDef : allVarDef)
        allASTVarDef.push_back(genASTVarDef(varDef));
    auto result = make_shared<ASTVarDecl>();
    result->type = cTypeToASTYType(declType->name);
    result->var_def_list = allASTVarDef;
    return result;
}

// VarDef : Ident LBRACKET_ConstExp_RBRACKET_Group | Ident LBRACKET_ConstExp_RBRACKET_Group ASSIGN InitVal
// LBRACKET_ConstExp_RBRACKET_Group : LBRACKET_ConstExp_RBRACKET_Group LBRACKET ConstExp RBRACKET |
shared_ptr<ASTVarDef> genASTVarDef(SyntaxTreeNode *varDef) {
    assert(varDef->name == "VarDef");
    auto ident = varDef->children[0];
    auto lBracketConstExpRBracketGroup = varDef->children[1];

    auto allConstExp = getListFromGroup(lBracketConstExpRBracketGroup, 2);
    vector<shared_ptr<ASTConstExp>> allAstConstExp;
    for (auto constExp : allConstExp)
        allAstConstExp.push_back(dealASTConstExp(constExp));

    auto result = make_shared<ASTVarDef>();
    result->id = ident->name;
    result->array_size = allAstConstExp;

    if (varDef->children.size() == 4) {
        auto initVal = varDef->children[3];
        auto astInitVal = genASTInitVal(initVal);
        result->init_val = astInitVal;
    } else
        result->init_val = nullptr;
    return result;
}

// InitVal : Exp | LBRACE InitVal_COMMA_InitVal_Group_Optional RBRACE
// InitVal_COMMA_InitVal_Group_Optional : InitVal COMMA_InitVal_Group |
// COMMA_InitVal_Group : COMMA_InitVal_Group COMMA InitVal |
shared_ptr<ASTInitVal> genASTInitVal(SyntaxTreeNode *initVal) {
    assert(initVal->name == "InitVal");
    if (initVal->children.size() == 1) {
        auto astExp = dealASTExp(initVal->children[0]);
        auto result = make_shared<ASTInitSingle>();
        result->exp = astExp;
        return result;
    } else {
        auto initValCommaInitValGroupOptional = initVal->children[1];
        auto result = make_shared<ASTInitList>();
        if (initValCommaInitValGroupOptional->children.size() > 0) {
            auto initVal1 = initValCommaInitValGroupOptional->children[0];
            auto commaInitValGroup = initValCommaInitValGroupOptional->children[1];
            auto allInitVal = getListFromGroup(commaInitValGroup, 2);
            allInitVal.insert(allInitVal.begin(), initVal1);

            vector<shared_ptr<ASTInitVal>> allAstInitVal;
            for (auto initVal : allInitVal)
                allAstInitVal.push_back(genASTInitVal(initVal));
            result->init_list = allAstInitVal;
        }
        return result;
    }
}

// FuncFParam : BType Ident LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional
// LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional : LBRACKET RBRACKET LBRACKET_Exp_RBRACKET_Group |
// LBRACKET_Exp_RBRACKET_Group : LBRACKET_Exp_RBRACKET_Group LBRACKET Exp RBRACKET |
shared_ptr<ASTFuncFParam> genASTFuncFParam(SyntaxTreeNode *funcFParam) {
    assert(funcFParam->name == "FuncFParam");
    auto fParamType = funcFParam->children[0]->children[0];
    auto ident = funcFParam->children[1];
    auto lBracketRBracketLBracketExpRBracketGroupOptional = funcFParam->children[2];

    auto result = make_shared<ASTFuncFParam>();
    result->type = cTypeToASTYType(fParamType->name);
    result->id = ident->name;
    if (lBracketRBracketLBracketExpRBracketGroupOptional->children.size() > 0) {
        result->is_array = true;
        auto lBracketExpRBracketGroup = lBracketRBracketLBracketExpRBracketGroupOptional->children[2];
        auto allExp = getListFromGroup(lBracketExpRBracketGroup, 2);
        auto allAstExp = vector<shared_ptr<ASTExp>>();
        for (auto exp : allExp)
            allAstExp.push_back(dealASTExp(exp));
        result->array_exp_list = allAstExp;
    } else
        result->is_array = false;
    return result;
}

// Block : LBRACE BlockItem_Group RBRACE
// BlockItem_Group : BlockItem_Group BlockItem |
shared_ptr<ASTBlock> genASTBlock(SyntaxTreeNode *block) {
    assert(block->name == "Block");
    auto blockItemGroup = block->children[1];
    auto allBlockItem = getListFromGroup(blockItemGroup, 1);
    auto allASTBlockItem = vector<shared_ptr<ASTBlockItem>>();
    for (auto blockItem : allBlockItem)
        allASTBlockItem.push_back(genASTBlockItem(blockItem));
    auto result = make_shared<ASTBlock>();
    result->item_list = allASTBlockItem;
    return result;
}

// BlockItem : Decl | Stmt
// Decl : ConstDecl | VarDecl
shared_ptr<ASTBlockItem> genASTBlockItem(SyntaxTreeNode *blockItem) {
    assert(blockItem->name == "BlockItem");
    auto result = make_shared<ASTBlockItem>();
    if (blockItem->children[0]->name == "Decl") {
        auto decl = blockItem->children[0];
        auto childDecl = decl->children[0];
        if (childDecl->name == "ConstDecl")
            result->const_decl = genASTConstDecl(childDecl);
        else
            result->var_decl = genASTVarDecl(childDecl);
    } else {
        auto stmt = blockItem->children[0];
        result->stmt = genASTStmt(stmt);
    }
    return result;
}

// # 其他函数

// Decl : ConstDecl | VarDecl
shared_ptr<ASTDeclDef> dealASTDecl(SyntaxTreeNode *decl) {
    assert(decl->name == "Decl");
    if (decl->children[0]->name == "ConstDecl") {
        auto result = genASTConstDecl(decl->children[0]);
        return result;
    } else {
        auto result = genASTVarDecl(decl->children[0]);
        return result;
    }
}

// Exp_Optional : Exp |
shared_ptr<ASTExp> dealASTExpOptional(SyntaxTreeNode *expOptional) {
    assert(expOptional->name == "Exp_Optional");
    if (expOptional->children.size() > 0) {
        auto exp = expOptional->children[0];
        auto result = dealASTExp(exp);
        return result;
    } else
        return nullptr;
}

// Exp : AddExp
shared_ptr<ASTExp> dealASTExp(SyntaxTreeNode *exp) {
    assert(exp->name == "Exp");
    auto addExp = exp->children[0];
    auto result = genASTAddExp(addExp);
    return result;
}

// ConstExp : AddExp
shared_ptr<ASTConstExp> dealASTConstExp(SyntaxTreeNode *constExp) {
    assert(constExp->name == "ConstExp");
    auto addExp = constExp->children[0];
    auto astAddExp = genASTAddExp(addExp);
    auto result = make_shared<ASTConstExp>();
    result->add_exp = astAddExp;
    return result;
}

// Cond : LOrExp
shared_ptr<ASTLOrExp> dealASTCond(SyntaxTreeNode *cond) {
    assert(cond->name == "Cond");
    auto lOrExp = cond->children[0];
    auto result = genASTLOrExp(lOrExp);
    return result;
}

// 从 Group 中获取所有的子节点
vector<SyntaxTreeNode *> getListFromGroup(SyntaxTreeNode *group, int targetIndex) {
    assert(group->name.find("Group") != string::npos);
    int groupIndex = 0;
    auto result = vector<SyntaxTreeNode *>();
    while (group->children.size() > 0) {
        result.push_back(group->children[targetIndex]);
        group = group->children[groupIndex];
    }
    std::reverse(result.begin(), result.end());
    return result;
}
