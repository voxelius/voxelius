add_library(client STATIC
    "${CMAKE_CURRENT_LIST_DIR}/client.cc"
    "${CMAKE_CURRENT_LIST_DIR}/game.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/input.cc"
    "${CMAKE_CURRENT_LIST_DIR}/player_look.cc"
    "${CMAKE_CURRENT_LIST_DIR}/player_move.cc"
    "${CMAKE_CURRENT_LIST_DIR}/screen.cc")
target_compile_definitions(client PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(client PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(client PUBLIC shared glad glfw)
