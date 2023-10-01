add_library(client STATIC
    "${CMAKE_CURRENT_LIST_DIR}/atlas.cc"
    "${CMAKE_CURRENT_LIST_DIR}/deferred_pass.cc"
    "${CMAKE_CURRENT_LIST_DIR}/game.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gbuffer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/image.cc"
    "${CMAKE_CURRENT_LIST_DIR}/input.cc"
    "${CMAKE_CURRENT_LIST_DIR}/main.cc"
    "${CMAKE_CURRENT_LIST_DIR}/player_look.cc"
    "${CMAKE_CURRENT_LIST_DIR}/player_move.cc"
    "${CMAKE_CURRENT_LIST_DIR}/postprocess.cc"
    "${CMAKE_CURRENT_LIST_DIR}/screen.cc"
    "${CMAKE_CURRENT_LIST_DIR}/screenshot.cc"
    "${CMAKE_CURRENT_LIST_DIR}/shaders.cc"
    "${CMAKE_CURRENT_LIST_DIR}/view.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_anims.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_mesher.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_vertex.cc")
target_compile_definitions(client PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(client PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(client PUBLIC shared)
target_link_libraries(client PUBLIC glad glfw stb)
