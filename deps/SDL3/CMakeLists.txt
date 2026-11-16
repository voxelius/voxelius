include(FetchContent)

if(WIN32)
    set(SDL_REQVER 3.4.12)

    if(MSVC)
        set(SDL3_DEVEL_KIT VC)
    elseif(MINGW)
        set(SDL3_DEVEL_KIT mingw)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
        set(SDL3_DEVEL_KIT VC)
    else()
        message(FATAL_ERROR "SDL3 doesn't provide Windows binaries for your build environment!")
    endif()

    FetchContent_Declare(download_sdl3 URL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL_REQVER}/SDL3-devel-${SDL_REQVER}-${SDL3_DEVEL_KIT}.zip" DOWNLOAD_EXTRACT_TIMESTAMP)
    FetchContent_MakeAvailable(download_sdl3)
    
    set(SDL3_DIR "${download_sdl3_SOURCE_DIR}/cmake")
endif()

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

## Interface the package into a target
## that can be used anywhere in the project
add_library(deps_SDL3 INTERFACE)
target_link_libraries(deps_SDL3 INTERFACE SDL3::SDL3)

## Copy over the DLL
## TODO: call install maybe?
if(WIN32 AND TARGET SDL3::SDL3)
    get_target_property(SDL3_DLL_LOCATION SDL3::SDL3 IMPORTED_LOCATION)

    if(SDL3_DLL_LOCATION AND EXISTS "${SDL3_DLL_LOCATION}")
        add_custom_target(deps_SDL3_DLL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL3_DLL_LOCATION}" "${PROJECT_BINARY_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL3_DLL_LOCATION}" "${PROJECT_SOURCE_DIR}"
            COMMENT "Copying SDL3 DLL" VERBATIM)
        add_dependencies(deps_SDL3 deps_SDL3_DLL)
    endif()

    install(FILES "${SDL3_DLL_LOCATION}" DESTINATION ".")
endif()

## Add an alias library so dependency list looks nicer
add_library(deps::SDL3 ALIAS deps_SDL3)
