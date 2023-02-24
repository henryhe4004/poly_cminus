%option noyywrap
%{
    #include <iostream>
    #include <string>
    #include <vector>

    #include "SyntaxTree.hh"
    #include "parser.hh"

    int line_number = 1;
    int column_start_number = 1;
    int column_end_number = 1;

    void pass_node(char *text){
        yylval.node = new_node(text,{});
    }

    void deal_one_line() {
        column_start_number = column_end_number;
        column_end_number += strlen(yytext);
        // printf("%d: %s\n", line_number, yytext);
        pass_node(yytext);
    }
%}


%%

\/\*([^\*]|\*[^\/])*\*+\/  {
    column_start_number = column_end_number;
    for (unsigned i = 0; i < strlen(yytext); i++) {
        if (yytext[i] == '\n') {
            line_number++;
            column_end_number = 1;
        } else
            column_end_number++;
    }
}

\/\/.*  {
    column_start_number = column_end_number;
    column_end_number += strlen(yytext);
}

int { deal_one_line(); return INT; }
float { deal_one_line(); return FLOAT; }
void { deal_one_line(); return VOID; }
const { deal_one_line(); return CONST; }
if { deal_one_line(); return IF; }
else { deal_one_line(); return ELSE; }
while { deal_one_line(); return WHILE; }
break { deal_one_line(); return BREAK; }
continue { deal_one_line(); return CONTINUE; }
return { deal_one_line(); return RETURN; }
for { deal_one_line(); return FOR; }

[a-zA-Z_][a-zA-Z_0-9]* { deal_one_line(); return Ident; }

0[xX][0-9a-fA-F]+ { deal_one_line(); return IntConst; }
0[0-7]+ { deal_one_line(); return IntConst; }
[0-9]+ { deal_one_line(); return IntConst; }

0[xX]([0-9a-fA-F]*\.[0-9a-fA-F]+|[0-9a-fA-F]+\.?)[pP][+-]?[0-9]+|([0-9]*\.[0-9]+|[0-9]+\.)([eE][+-]?[0-9]+)?|[0-9]+([eE][+-]?[0-9]+)  { deal_one_line(); return floatConst; }


"+" { deal_one_line(); return ADD; }
"-" { deal_one_line(); return SUB; }
"*" { deal_one_line(); return MUL; }
"/" { deal_one_line(); return DIV; }
"%" { deal_one_line(); return MOD; }

"(" { deal_one_line(); return LPAREN; }
")" { deal_one_line(); return RPAREN; }
"[" { deal_one_line(); return LBRACKET; }
"]" { deal_one_line(); return RBRACKET; }
"{" { deal_one_line(); return LBRACE; }
"}" { deal_one_line(); return RBRACE; }

"<" { deal_one_line(); return LESS; }
"<=" { deal_one_line(); return LESS_EQUAL; }
">" { deal_one_line(); return GREATER; }
">=" { deal_one_line(); return GREATER_EQUAL; }
"==" { deal_one_line(); return EQUAL; }
"!=" { deal_one_line(); return NOT_EQUAL; }

"&&" { deal_one_line(); return AND; }
"||" { deal_one_line(); return OR; }
"!" { deal_one_line(); return NOT; }

"=" { deal_one_line(); return ASSIGN; }
"+=" { deal_one_line(); return ADD_ASSIGN; }
"," { deal_one_line(); return COMMA; }
";" { deal_one_line(); return SEMICOLON; }



\n {
    line_number++;
    column_start_number = column_end_number;
    column_end_number = 1;
}

" "|\r {
    column_start_number = column_end_number;
    column_end_number += strlen(yytext);
}

\t {
    column_start_number = column_end_number;
    column_end_number += 4;
}

. { deal_one_line(); return ERROR; }

%%
