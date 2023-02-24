file (GLOB OPT_SOURCES "*.cc")
add_library(optimize STATIC
    ${OPT_SOURCES}
)
