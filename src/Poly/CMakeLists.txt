# link with isl here
file (GLOB POLY_SOURCES "*.cc")
add_library(polyhedral STATIC
    # ${POLY_SOURCES}
    test.cc
    PolyBuilder.cc
    Polyhedral.cc
)

target_include_directories(
    polyhedral
    PUBLIC
    ${CMAKE_BINARY_DIR}/isl/include
    ../../include/MIR_new
    ../../include/Builder
    ../../include/AST
)
target_link_libraries(
    polyhedral
    ${CMAKE_BINARY_DIR}/isl/lib/libisl.a
    gmp # TODO: get our own gmp
    parser
    AST
)
