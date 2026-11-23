cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(Voxelius LANGUAGES C CXX VERSION 17.0.0)

option(ENABLE_PROFILING "Enable profiling" OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Emit compile_commands.json for clangd and other tooling. Only the Ninja and
# Makefile generators support it.
if(CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

if(MSVC)
    add_compile_options(/MP)
    add_compile_options(/W3)
endif()

if(ENABLE_PROFILING)
    add_compile_definitions(TRACY_ENABLE)
    add_compile_definitions(TRACY_ON_DEMAND)
    add_compile_definitions(TRACY_NO_FRAME_IMAGE)
endif()

execute_process(COMMAND git rev-parse --short=8 HEAD WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND git rev-parse --abbrev-ref HEAD WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

add_subdirectory(core)
add_subdirectory(deps)
add_subdirectory(shared)
add_subdirectory(client)
add_subdirectory(server)

# clangd looks for the compilation database in the repository root, while CMake writes it into the
# binary dir; refresh the root copy on every build (copy_if_different compares content, so an
# unchanged database is not rewritten). The Visual Studio generator cannot export one at all.
if(CMAKE_EXPORT_COMPILE_COMMANDS AND CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
    add_custom_target(copy-compile-commands ALL
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${PROJECT_BINARY_DIR}/compile_commands.json"
                "${PROJECT_SOURCE_DIR}/compile_commands.json"
        COMMENT "Refreshing ${PROJECT_SOURCE_DIR}/compile_commands.json for clangd"
        VERBATIM)
endif()

install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" DESTINATION ".")
install(DIRECTORY "${PROJECT_SOURCE_DIR}/data" DESTINATION ".")

set(CPACK_PACKAGE_NAME "Voxelius")
set(CPACK_PACKAGE_VENDOR "voxelius")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}-${GIT_BRANCH}-${GIT_COMMIT}")
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_SOURCE_DIR}/dist")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}\\\\media\\\\nsis-installer.ico")
set(CPACK_NSIS_MUI_HEADERIMAGE "${PROJECT_SOURCE_DIR}\\\\media\\\\nsis-header.bmp")
set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${PROJECT_SOURCE_DIR}\\\\media\\\\nsis-splash.bmp")

unset(CPACK_NSIS_MENU_LINKS)
list(APPEND CPACK_NSIS_MENU_LINKS "bin/vclient.exe" "Voxelius")
list(APPEND CPACK_NSIS_MENU_LINKS "bin/vserver.exe" "Voxelius server")

include(CPack)
