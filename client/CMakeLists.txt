add_library(client STATIC
    "${CMAKE_CURRENT_LIST_DIR}/main.cc")
target_compile_definitions(client PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(client PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(client PUBLIC shared)
target_link_libraries(client PUBLIC glfw imgui imgui_glfw imgui_opengl3 stb)
target_link_libraries(client PUBLIC OpenGL::GL)
