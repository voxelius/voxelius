option(BUILD_CLIENT "Build client" ON)
option(BUILD_SERVER "Build server" OFF)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(shared)

if(BUILD_CLIENT)
    add_executable(vgame "${CMAKE_CURRENT_LIST_DIR}/game.cc")
    target_compile_definitions(vgame PUBLIC VGAME_CLIENT)
    target_include_directories(vgame PUBLIC "${CMAKE_SOURCE_DIR}")
    target_link_libraries(vgame PUBLIC physfs spdlog)
    target_link_libraries(vgame PUBLIC client)
endif()
