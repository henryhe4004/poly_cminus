# flex_target(lex lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
# bison_target(syntax parser.y
#   ${CMAKE_CURRENT_BINARY_DIR}/parser.cc
#   DEFINES_FILE ${PROJECT_BINARY_DIR}/parser.hh)

add_library(parser STATIC
    lexer.cc
    parser.cc
    SyntaxAnalyze.cc
)
