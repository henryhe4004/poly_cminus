cmake_minimum_required(VERSION 3.16)
project(SYSYCompiler)
set(CMAKE_CXX_STANDARD 17)

# setting build types
include (cmake/build_type.cmake)
# # clang-format
# include (cmake/clang-format.cmake)
# flex, bison, llvm, boost
include (cmake/libraries.cmake)
# PROJECT_BINARY_DIR=全路径/build
include_directories(${PROJECT_BINARY_DIR})
include_directories(include)
include_directories(include/AST)
include_directories(include/Builder)
include_directories(include/codegen_new)
include_directories(include/parser)
include_directories(include/Optimize)
include_directories(include/MIR_new)
include_directories(include/syntax_tree)

add_subdirectory(lib)
add_subdirectory(src/Builder)
add_subdirectory(src/MIR_new)
add_subdirectory(src/parser)
add_subdirectory(src/Optimize)
add_subdirectory(src/codegen_new)
add_subdirectory(src/common)
add_subdirectory(src/LIR)
add_subdirectory(test)
add_library(logging STATIC src/logging.cpp)
add_library(utils STATIC src/utils.cc)

add_executable(
        cc
        src/compiler.cc
)

target_link_libraries(
        cc
        utils
        #parser
        MIR_lib_new
        IRBuilder_new
        optimize
        codegen_new
        syntax
        # polyhedral
        LIR
        utils
        logging
        common
)
