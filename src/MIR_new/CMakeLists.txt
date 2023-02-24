add_library(
        MIR_lib_new STATIC
        Type.cc
        User.cc
        Value.cc
        BasicBlock.cc
        Constant.cc
        Function.cc
        GlobalVariable.cc
        Instruction.cc
        Module.cc
        IRprinter.cc
)

target_include_directories(
        MIR_lib_new
        PUBLIC
        ../../include/MIR_new
)
