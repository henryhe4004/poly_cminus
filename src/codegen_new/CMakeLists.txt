add_library(
    codegen_new
    codegen.cc
    regalloc.cc
    instgen.cc
)

target_include_directories(
    codegen_new
    PUBLIC
    ../../include/codegen_new
    ../../include/MIR_new
)
