add_library(client STATIC
    "${CMAKE_CURRENT_LIST_DIR}/entity/player_look.cc"
    "${CMAKE_CURRENT_LIST_DIR}/entity/player_move.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/program.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/shader.cc"
    "${CMAKE_CURRENT_LIST_DIR}/chunk_mesher.cc"
    "${CMAKE_CURRENT_LIST_DIR}/chunk_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/chunk_visibility.cc"
    "${CMAKE_CURRENT_LIST_DIR}/font.cc"
    "${CMAKE_CURRENT_LIST_DIR}/game.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/keyboard.cc"
    "${CMAKE_CURRENT_LIST_DIR}/keynames.cc"
    "${CMAKE_CURRENT_LIST_DIR}/main.cc"
    "${CMAKE_CURRENT_LIST_DIR}/mouse.cc"
    "${CMAKE_CURRENT_LIST_DIR}/outline_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/player_target.cc"
    "${CMAKE_CURRENT_LIST_DIR}/screenshot.cc"
    "${CMAKE_CURRENT_LIST_DIR}/text_builder.cc"
    "${CMAKE_CURRENT_LIST_DIR}/text_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/text_vbo.cc"
    "${CMAKE_CURRENT_LIST_DIR}/view.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_anims.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_atlas.cc")
target_compile_definitions(client PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(client PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(client PUBLIC glad glfw schrift)
target_link_libraries(client PUBLIC shared)
