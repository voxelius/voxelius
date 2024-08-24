add_library(client STATIC
    "${CMAKE_CURRENT_LIST_DIR}/entity/player_look.cc"
    "${CMAKE_CURRENT_LIST_DIR}/entity/player_move.cc"
    "${CMAKE_CURRENT_LIST_DIR}/background.cc"
    "${CMAKE_CURRENT_LIST_DIR}/chunk_mesher.cc"
    "${CMAKE_CURRENT_LIST_DIR}/chunk_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/chunk_visibility.cc"
    "${CMAKE_CURRENT_LIST_DIR}/debug_screen.cc"
    "${CMAKE_CURRENT_LIST_DIR}/debug_session.cc"
    "${CMAKE_CURRENT_LIST_DIR}/debug_toggles.cc"
    "${CMAKE_CURRENT_LIST_DIR}/game.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/keyboard.cc"
    "${CMAKE_CURRENT_LIST_DIR}/keynames.cc"
    "${CMAKE_CURRENT_LIST_DIR}/language.cc"
    "${CMAKE_CURRENT_LIST_DIR}/main_menu.cc"
    "${CMAKE_CURRENT_LIST_DIR}/main.cc"
    "${CMAKE_CURRENT_LIST_DIR}/message_box.cc"
    "${CMAKE_CURRENT_LIST_DIR}/mouse.cc"
    "${CMAKE_CURRENT_LIST_DIR}/outline_renderer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/player_target.cc"
    "${CMAKE_CURRENT_LIST_DIR}/progress.cc"
    "${CMAKE_CURRENT_LIST_DIR}/receive.cc"
    "${CMAKE_CURRENT_LIST_DIR}/screenshot.cc"
    "${CMAKE_CURRENT_LIST_DIR}/server_list.cc"
    "${CMAKE_CURRENT_LIST_DIR}/session.cc"
    "${CMAKE_CURRENT_LIST_DIR}/settings.cc"
    "${CMAKE_CURRENT_LIST_DIR}/varied_program.cc"
    "${CMAKE_CURRENT_LIST_DIR}/view.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_anims.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_atlas.cc")
target_compile_definitions(client PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(client PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(client PUBLIC glad glfw imgui imgui_glfw imgui_opengl3)
target_link_libraries(client PUBLIC shared)

if(WIN32 AND MSVC)
    # GLFW defines APIENTRY and ENet includes
    # Windows API headers which also define APIENTRY
    target_compile_options(client PRIVATE /wd4005)
endif()
