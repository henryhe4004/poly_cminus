add_library(
    IRBuilder_new
    cminusf_builder.cc
)

target_include_directories(
    IRBuilder_new
    PUBLIC
    ../../include/MIR_new
    ../../include/Builder
    ../../include/AST
)

target_link_libraries(
    IRBuilder_new
    MIR_lib_new
)
# target_link_libraries(
#     IRBuilder_new
#     MIR_lib_new
# )
