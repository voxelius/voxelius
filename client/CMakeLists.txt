add_library(client STATIC
    "${CMAKE_CURRENT_LIST_DIR}/entity/player_look.cc"
    "${CMAKE_CURRENT_LIST_DIR}/entity/player_move.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gui/bitmap_font.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gui/text_builder.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gui/text_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gui/text_vbo.cc"
    "${CMAKE_CURRENT_LIST_DIR}/input/keyboard.cc"
    "${CMAKE_CURRENT_LIST_DIR}/input/keynames.cc"
    "${CMAKE_CURRENT_LIST_DIR}/input/mouse.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/shader.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/program.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/chunk_mesher.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/chunk_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/chunk_visibility.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/outline_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/player_target.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/voxel_anims.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/voxel_atlas.cc"
    "${CMAKE_CURRENT_LIST_DIR}/game.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/main.cc"
    "${CMAKE_CURRENT_LIST_DIR}/screenshot.cc"
    "${CMAKE_CURRENT_LIST_DIR}/view.cc")
target_compile_definitions(client PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(client PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(client PUBLIC glad glfw imgui imgui_glfw imgui_opengl3)
target_link_libraries(client PUBLIC shared)
