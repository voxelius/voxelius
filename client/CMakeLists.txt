add_library(client STATIC
    "${CMAKE_CURRENT_LIST_DIR}/debug/debug_draw.cc"
    "${CMAKE_CURRENT_LIST_DIR}/debug/debug_screen.cc"
    "${CMAKE_CURRENT_LIST_DIR}/debug/debug_session.cc"
    "${CMAKE_CURRENT_LIST_DIR}/debug/debug_toggles.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gameui/background.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gameui/language.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gameui/main_menu.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gameui/progress.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gameui/server_list.cc"
    "${CMAKE_CURRENT_LIST_DIR}/gameui/settings.cc"
    "${CMAKE_CURRENT_LIST_DIR}/input/keyboard.cc"
    "${CMAKE_CURRENT_LIST_DIR}/input/mouse.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/keyname.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/shadertools.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/chunk_mesher.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/chunk_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world/chunk_visibility.cc"
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
