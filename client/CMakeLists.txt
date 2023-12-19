add_library(client STATIC
    "${CMAKE_CURRENT_LIST_DIR}/background.cc"
    "${CMAKE_CURRENT_LIST_DIR}/camera.cc"
    "${CMAKE_CURRENT_LIST_DIR}/debug_session.cc"
    "${CMAKE_CURRENT_LIST_DIR}/game.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/image.cc"
    "${CMAKE_CURRENT_LIST_DIR}/lang.cc"
    "${CMAKE_CURRENT_LIST_DIR}/main.cc"
    "${CMAKE_CURRENT_LIST_DIR}/player_look.cc"
    "${CMAKE_CURRENT_LIST_DIR}/player_move.cc"
    "${CMAKE_CURRENT_LIST_DIR}/screenshot.cc"
    "${CMAKE_CURRENT_LIST_DIR}/shaders.cc"
    "${CMAKE_CURRENT_LIST_DIR}/ui_main_menu.cc"
    "${CMAKE_CURRENT_LIST_DIR}/ui_server_list.cc"
    "${CMAKE_CURRENT_LIST_DIR}/ui_settings.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_anims.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_atlas.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_mesher.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_vertex.cc")
target_compile_definitions(client PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(client PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(client PUBLIC shared)
target_link_libraries(client PUBLIC glad glfw imgui imgui_glfw imgui_opengl3 stb)
