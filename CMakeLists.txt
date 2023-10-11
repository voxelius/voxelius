cmake_minimum_required(VERSION 3.14)
project(VOXELIUS LANGUAGES CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Having binaries directly in the build
# directory makes it easier to write portable
# scripts for making packaged releases, I guess.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# We like our stuff to be as monolithic as possible
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Enable commonly used formats
set(PHYSFS_ARCHIVE_ISO9660 OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_ZIP ON CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_7Z ON CACHE BOOL "" FORCE)

# Disable obscure or legacy formats
set(PHYSFS_ARCHIVE_QPAK OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_GRP OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_WAD OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_HOG OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_MVL OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_SLB OFF CACHE BOOL "" FORCE)
set(PHYSFS_ARCHIVE_VDF OFF CACHE BOOL "" FORCE)

if(WIN32)
    include(FetchContent)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw3
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3-stable
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE)
    FetchContent_MakeAvailable(glfw3)
else()
    # On non-Windows systems it's assumed
    # that the GLFW3 is bundled in with the
    # system package manager
    find_package(glfw3 3.3 REQUIRED)
endif()

add_subdirectory(deps/entt)
add_subdirectory(deps/glad)
add_subdirectory(deps/glm)
add_subdirectory(deps/parson)
add_subdirectory(deps/physfs)
add_subdirectory(deps/spdlog)
add_subdirectory(deps/stb)
add_subdirectory(deps/thread_pool)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(shared)

add_subdirectory(main)
