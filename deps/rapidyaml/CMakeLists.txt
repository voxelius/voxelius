add_library(deps_rapidyaml STATIC
    "${CMAKE_CURRENT_LIST_DIR}/include/ryml.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/ryml.cpp")
target_include_directories(deps_rapidyaml PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
target_compile_definitions(deps_rapidyaml PUBLIC
    RYML_DEFAULT_CALLBACK_USES_EXCEPTIONS=1
    RYML_WITH_TAB_TOKENS=1)

add_library(deps::ryml ALIAS deps_rapidyaml)
