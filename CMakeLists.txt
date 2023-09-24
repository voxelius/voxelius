## SPDX-License-Identifier: MPL-2.0
## This Source Code Form is subject to the terms of the Mozilla Public
## License, v2.0. If a copy of the MPL was not distributed with this
## file, You can obtain one at http://mozilla.org/MPL/2.0/.
cmake_minimum_required(VERSION 3.10)
project(VOXELIUS LANGUAGES CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Having binaries directly in the build
# directory makes it easier to write portable
# scripts for making packaged releases, I guess.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

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

add_subdirectory(deps/entt)
add_subdirectory(deps/glad)
add_subdirectory(deps/glm)
add_subdirectory(deps/parson)
add_subdirectory(deps/physfs)
add_subdirectory(deps/spdlog)
add_subdirectory(deps/stb)
add_subdirectory(deps/thread_pool)
find_package(glfw3 3.3 REQUIRED)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(shared)

add_subdirectory(main)
