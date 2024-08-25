add_library(mathlib STATIC
    "${CMAKE_CURRENT_LIST_DIR}/mat4x4f.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vec2f.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vec3angles.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vec3f.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vec4f.cc")
target_compile_definitions(mathlib PUBLIC NOMINMAX)
target_include_directories(mathlib PUBLIC ${CMAKE_SOURCE_DIR})
