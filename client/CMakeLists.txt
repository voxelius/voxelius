add_library(client STATIC
    "${CMAKE_CURRENT_LIST_DIR}/player/player_look.cc"
    "${CMAKE_CURRENT_LIST_DIR}/player/player_move.cc"
    "${CMAKE_CURRENT_LIST_DIR}/render/canvas.cc"
    "${CMAKE_CURRENT_LIST_DIR}/render/deferred_pass.cc"
    "${CMAKE_CURRENT_LIST_DIR}/render/final_pass.cc"
    "${CMAKE_CURRENT_LIST_DIR}/render/font.cc"
    "${CMAKE_CURRENT_LIST_DIR}/render/gbuffer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/render/shaders.cc"
    "${CMAKE_CURRENT_LIST_DIR}/render/text.cc"
    "${CMAKE_CURRENT_LIST_DIR}/render/voxel_anims.cc"
    "${CMAKE_CURRENT_LIST_DIR}/render/voxel_atlas.cc"
    "${CMAKE_CURRENT_LIST_DIR}/render/voxel_mesher.cc"
    "${CMAKE_CURRENT_LIST_DIR}/render/voxel_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/render/voxel_vertex.cc"
    "${CMAKE_CURRENT_LIST_DIR}/ui/imgui.cc"
    "${CMAKE_CURRENT_LIST_DIR}/ui/main_menu.cc"
    "${CMAKE_CURRENT_LIST_DIR}/game.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/image.cc"
    "${CMAKE_CURRENT_LIST_DIR}/main.cc"
    "${CMAKE_CURRENT_LIST_DIR}/view.cc")
target_compile_definitions(client PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(client PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(client PUBLIC shared)
target_link_libraries(client PUBLIC glad glfw stb)
