add_library(client STATIC
    "${CMAKE_CURRENT_LIST_DIR}/util/program.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/shader.cc"
    "${CMAKE_CURRENT_LIST_DIR}/atlas.cc"
    "${CMAKE_CURRENT_LIST_DIR}/background.cc"
    "${CMAKE_CURRENT_LIST_DIR}/camera.cc"
    "${CMAKE_CURRENT_LIST_DIR}/chunk_mesher.cc"
    "${CMAKE_CURRENT_LIST_DIR}/chunk_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/chunk_vis.cc"
    "${CMAKE_CURRENT_LIST_DIR}/debug_session.cc"
    "${CMAKE_CURRENT_LIST_DIR}/game.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/key_name.cc"
    "${CMAKE_CURRENT_LIST_DIR}/keyboard.cc"
    "${CMAKE_CURRENT_LIST_DIR}/lang.cc"
    "${CMAKE_CURRENT_LIST_DIR}/main_menu.cc"
    "${CMAKE_CURRENT_LIST_DIR}/main.cc"
    "${CMAKE_CURRENT_LIST_DIR}/mouse.cc"
    "${CMAKE_CURRENT_LIST_DIR}/progress.cc"
    "${CMAKE_CURRENT_LIST_DIR}/screenshot.cc"
    "${CMAKE_CURRENT_LIST_DIR}/server_list.cc"
    "${CMAKE_CURRENT_LIST_DIR}/settings.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_anims.cc")
target_compile_definitions(client PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(client PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(client PUBLIC glad glfw imgui imgui_glfw imgui_opengl3)
target_link_libraries(client PUBLIC shared)
