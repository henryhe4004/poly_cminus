flex_target(lex lexical_analyzer.l ${CMAKE_CURRENT_BINARY_DIR}/lexical_analyzer.c)

# CMAKE_CURRENT_BINARY_DIR  当前build目录
bison_target(syntax syntax_analyzer.y ${CMAKE_CURRENT_BINARY_DIR}/syntax_analyzer.c DEFINES_FILE ${PROJECT_BINARY_DIR}/syntax_analyzer.h)

add_flex_bison_dependency(lex syntax)

add_library(syntax STATIC
  ${BISON_syntax_OUTPUTS}
  ${FLEX_lex_OUTPUTS}
)
