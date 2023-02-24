# set(BOOST_ROOT third_party/boost CACHE PATH "Boost library path" )
set(BOOST_INCLUDEDIR third_party/include)
# set(Boost_NO_SYSTEM_PATHS on CACHE BOOL "Do not search system for Boost" )
set(Boost_NO_BOOST_CMAKE TRUE)
message(STATUS "${BOOST_ROOT}")
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS} SYSTEM)

# Import LLVM
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Boost include path ${Boost_INCLUDE_DIRS}")

llvm_map_components_to_libnames(
        llvm_libs
        all
)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
