cmake_minimum_required(VERSION 3.14)
project(VOXELIUS LANGUAGES CXX VERSION 0.0.1)

set(BUILD_CLIENT ON CACHE BOOL "Build client executable")
set(BUILD_SERVER ON CACHE BOOL "Build server executable")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_ZIP ON CACHE BOOL "" FORCE)

if(BUILD_CLIENT)
    add_subdirectory(external/glad)
    add_subdirectory(external/imgui)
    find_package(glfw3 QUIET)

    if(NOT glfw3_FOUND)
        include(FetchContent)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(glfw3
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE)
        FetchContent_MakeAvailable(glfw3)
    endif()
endif()

add_subdirectory(external/emhash)
add_subdirectory(external/enet)
add_subdirectory(external/entt)
add_subdirectory(external/fastnoiselite)
add_subdirectory(external/parson)
add_subdirectory(external/physfs)
add_subdirectory(external/spdlog)
add_subdirectory(external/stb)
add_subdirectory(external/thread_pool)

add_subdirectory(common)
add_subdirectory(config)
add_subdirectory(mathlib)

add_subdirectory(game/client)
add_subdirectory(game/server)
add_subdirectory(game/shared)
add_subdirectory(launch)
