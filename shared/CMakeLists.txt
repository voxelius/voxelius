add_library(shared STATIC
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/inertial.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vdef.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world.cc")
target_include_directories(shared PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(shared PUBLIC core)
target_link_libraries(shared PUBLIC entt parson)
