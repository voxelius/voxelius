add_library(server STATIC
    "${CMAKE_CURRENT_LIST_DIR}/game.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/main.cc"
    "${CMAKE_CURRENT_LIST_DIR}/network.cc"
    "${CMAKE_CURRENT_LIST_DIR}/session.cc"
    "${CMAKE_CURRENT_LIST_DIR}/status.cc")
target_include_directories(server PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(server PUBLIC shared)
