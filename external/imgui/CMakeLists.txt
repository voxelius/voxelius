cmake_minimum_required(VERSION 3.15)
project(imgui LANGUAGES CXX VERSION 1.90.1)

add_library(imgui STATIC
    "${CMAKE_CURRENT_LIST_DIR}/src/imgui_demo.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/imgui_draw.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/imgui_stdlib.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/imgui_tables.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/imgui_widgets.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/imgui.cpp")
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
target_include_directories(imgui PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")

add_library(imgui_glfw STATIC "${CMAKE_CURRENT_LIST_DIR}/src/imgui_impl_glfw.cpp")
target_link_libraries(imgui_glfw PUBLIC glfw imgui)

add_library(imgui_opengl3 STATIC "${CMAKE_CURRENT_LIST_DIR}/src/imgui_impl_opengl3.cpp")
target_link_libraries(imgui_opengl3 PUBLIC imgui)
