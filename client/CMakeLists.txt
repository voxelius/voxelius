add_library(client STATIC
    "${CMAKE_CURRENT_LIST_DIR}/entity/player_look.cc"
    "${CMAKE_CURRENT_LIST_DIR}/entity/player_move.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gui/background.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gui/language.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gui/main_menu.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gui/progress.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gui/server_list.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gui/settings.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/shader.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/program.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/chunk_mesher.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/chunk_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/chunk_visibility.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/player_target.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/voxel_anims.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/voxel_atlas.cc"
    "${CMAKE_CURRENT_LIST_DIR}/game.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/keyboard.cc"
    "${CMAKE_CURRENT_LIST_DIR}/keynames.cc"
    "${CMAKE_CURRENT_LIST_DIR}/main.cc"
    "${CMAKE_CURRENT_LIST_DIR}/mouse.cc"
    "${CMAKE_CURRENT_LIST_DIR}/screenshot.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vdraw.cc"
    "${CMAKE_CURRENT_LIST_DIR}/view.cc")
target_compile_definitions(client PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(client PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(client PUBLIC glad glfw imgui imgui_glfw imgui_opengl3)
target_link_libraries(client PUBLIC shared)
