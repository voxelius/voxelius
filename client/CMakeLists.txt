add_library(client STATIC
    "${CMAKE_CURRENT_LIST_DIR}/client.cc"
    "${CMAKE_CURRENT_LIST_DIR}/game.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/image.cc"
    "${CMAKE_CURRENT_LIST_DIR}/input.cc"
    "${CMAKE_CURRENT_LIST_DIR}/plook.cc"
    "${CMAKE_CURRENT_LIST_DIR}/pmove.cc"
    "${CMAKE_CURRENT_LIST_DIR}/screen.cc"
    "${CMAKE_CURRENT_LIST_DIR}/view.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_mesher.cc")
target_compile_definitions(client PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(client PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(client PUBLIC glad glfw stb)
target_link_libraries(client PUBLIC shared)
