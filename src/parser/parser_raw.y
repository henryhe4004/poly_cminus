%{
    #include <iostream>
    #include <string>
    #include <vector>

    #include "SyntaxTree.hh"

    extern int yylex();
    extern int yyparse();
    extern int yyrestart();
    extern FILE* yyin;

    extern char* yytext;
    extern int line_number;
    extern int column_end_number;
    extern int column_start_number;

    extern SyntaxTree syntax_tree;

    void yyerror(const char *s) {
        std::cerr << s << std::endl;
        std::cerr << "Error at line " << line_number << ": " << column_end_number << std::endl;
        std::cerr << "Error: " << yytext << std::endl;
        std::abort();
    }
%}

%union {
    class SyntaxTreeNode *node;
}

%token <node> INT FLOAT VOID CONST IF ELSE WHILE BREAK CONTINUE RETURN FOR
%token <node> Ident IntConst floatConst
%token <node> ADD SUB MUL DIV MOD
%token <node> LPAREN RPAREN LBRACKET RBRACKET LBRACE RBRACE
%token <node> LESS LESS_EQUAL GREATER GREATER_EQUAL EQUAL NOT_EQUAL
%token <node> AND OR NOT
%token <node> ASSIGN ADD_ASSIGN COMMA SEMICOLON
%token <node> ERROR

%type <node> CompUnit Decl ConstDecl BType ConstDef ConstInitVal VarDecl VarDef InitVal
%type <node> FuncDef FuncFParams FuncFParam Block BlockItem Stmt Exp Cond LVal PrimaryExp
%type <node> Number UnaryExp UnaryOp FuncRParams MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp
%type <node> Decl_or_FuncDef MulOp AddOp
%type <node> RelOp EqOp COMMA_ConstDef_Group
%type <node> LBRACKET_ConstExp_RBRACKET_Group COMMA_ConstInitVal_Group COMMA_VarDef_Group
%type <node> COMMA_InitVal_Group COMMA_FuncFParam_Group LBRACKET_Exp_RBRACKET_Group BlockItem_Group
%type <node> COMMA_Exp_Group CompUnit_Optional ConstInitVal_COMMA_ConstInitVal_Group_Optional
%type <node> InitVal_COMMA_InitVal_Group_Optional FuncFParams_Optional
%type <node> LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional Exp_Optional FuncRParams_Optional
%type <node> ELSE_Stmt_Optional

%start CompUnit

%%
CompUnit        : CompUnit_Optional Decl_or_FuncDef { $$ = new_node("CompUnit", {$1, $2}); syntax_tree.root = $$; };
Decl            : ConstDecl | VarDecl ;
ConstDecl       : CONST BType ConstDef COMMA_ConstDef_Group SEMICOLON ;
BType           : INT | FLOAT | VOID ;
ConstDef        : Ident LBRACKET_ConstExp_RBRACKET_Group ASSIGN ConstInitVal ;
ConstInitVal    : ConstExp | LBRACE ConstInitVal_COMMA_ConstInitVal_Group_Optional RBRACE ;
VarDecl         : BType VarDef COMMA_VarDef_Group SEMICOLON ;
VarDef          : Ident LBRACKET_ConstExp_RBRACKET_Group | Ident LBRACKET_ConstExp_RBRACKET_Group ASSIGN InitVal ;
InitVal         : Exp | LBRACE InitVal_COMMA_InitVal_Group_Optional RBRACE ;
FuncDef         : BType Ident LPAREN FuncFParams_Optional RPAREN Block ;
FuncFParams     : FuncFParam COMMA_FuncFParam_Group ;
FuncFParam      : BType Ident LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional ;
Block           : LBRACE BlockItem_Group RBRACE ;
BlockItem       : Decl | Stmt ;
Stmt            : LVal ASSIGN Exp SEMICOLON | Exp_Optional SEMICOLON | Block | IF LPAREN Cond RPAREN Stmt ELSE_Stmt_Optional | WHILE LPAREN Cond RPAREN Stmt | BREAK SEMICOLON | CONTINUE SEMICOLON | RETURN Exp_Optional SEMICOLON | FOR LPAREN BType LVal ASSIGN Exp SEMICOLON Cond SEMICOLON LVal ADD_ASSIGN Exp RPAREN Stmt;
Exp             : AddExp ;
Cond            : LOrExp ;
LVal            : Ident LBRACKET_Exp_RBRACKET_Group ;
PrimaryExp      : LPAREN Exp RPAREN | LVal | Number ;
Number          : IntConst | floatConst ;
UnaryExp        : PrimaryExp | Ident LPAREN FuncRParams_Optional RPAREN | UnaryOp UnaryExp ;
UnaryOp         : ADD | SUB | NOT ;
FuncRParams     : Exp COMMA_Exp_Group ;
MulExp          : UnaryExp | MulExp MulOp UnaryExp ;
AddExp          : MulExp | AddExp AddOp MulExp ;
RelExp          : AddExp | RelExp RelOp AddExp ;
EqExp           : RelExp | EqExp EqOp RelExp ;
LAndExp         : EqExp | LAndExp AND EqExp ;
LOrExp          : LAndExp | LOrExp OR LAndExp ;
ConstExp        : AddExp ;
Decl_or_FuncDef : Decl | FuncDef ;
MulOp           : MUL | DIV | MOD ;
AddOp           : ADD | SUB ;
RelOp           : LESS | GREATER | LESS_EQUAL | GREATER_EQUAL ;
EqOp            : EQUAL | NOT_EQUAL ;
COMMA_ConstDef_Group                                    : COMMA_ConstDef_Group COMMA ConstDef | ;
LBRACKET_ConstExp_RBRACKET_Group                        : LBRACKET_ConstExp_RBRACKET_Group LBRACKET ConstExp RBRACKET | ;
COMMA_ConstInitVal_Group                                : COMMA_ConstInitVal_Group COMMA ConstInitVal | ;
COMMA_VarDef_Group                                      : COMMA_VarDef_Group COMMA VarDef | ;
COMMA_InitVal_Group                                     : COMMA_InitVal_Group COMMA InitVal | ;
COMMA_FuncFParam_Group                                  : COMMA_FuncFParam_Group COMMA FuncFParam | ;
LBRACKET_Exp_RBRACKET_Group                             : LBRACKET_Exp_RBRACKET_Group LBRACKET Exp RBRACKET | ;
BlockItem_Group                                         : BlockItem_Group BlockItem | ;
COMMA_Exp_Group                                         : COMMA_Exp_Group COMMA Exp | ;
CompUnit_Optional                                       : CompUnit | ;
ConstInitVal_COMMA_ConstInitVal_Group_Optional          : ConstInitVal COMMA_ConstInitVal_Group | ;
InitVal_COMMA_InitVal_Group_Optional                    : InitVal COMMA_InitVal_Group | ;
FuncFParams_Optional                                    : FuncFParams | ;
LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional  : LBRACKET RBRACKET LBRACKET_Exp_RBRACKET_Group | ;
Exp_Optional                                            : Exp | ;
FuncRParams_Optional                                    : FuncRParams | ;
ELSE_Stmt_Optional                                      : ELSE Stmt | ;
%%
