add_library(shared STATIC
    "${CMAKE_CURRENT_LIST_DIR}/entity/transform.cc"
    "${CMAKE_CURRENT_LIST_DIR}/entity/velocity.cc"
    "${CMAKE_CURRENT_LIST_DIR}/math/mat4x4f.cc"
    "${CMAKE_CURRENT_LIST_DIR}/math/vec2f.cc"
    "${CMAKE_CURRENT_LIST_DIR}/math/vec3angles.cc"
    "${CMAKE_CURRENT_LIST_DIR}/math/vec3f.cc"
    "${CMAKE_CURRENT_LIST_DIR}/math/vec4f.cc"
    "${CMAKE_CURRENT_LIST_DIR}/cmdline.cc"
    "${CMAKE_CURRENT_LIST_DIR}/config.cc"
    "${CMAKE_CURRENT_LIST_DIR}/crc64.cc"
    "${CMAKE_CURRENT_LIST_DIR}/epoch.cc"
    "${CMAKE_CURRENT_LIST_DIR}/fstools.cc"
    "${CMAKE_CURRENT_LIST_DIR}/game_voxels.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/image.cc"
    "${CMAKE_CURRENT_LIST_DIR}/protocol.cc"
    "${CMAKE_CURRENT_LIST_DIR}/ray_dda.cc"
    "${CMAKE_CURRENT_LIST_DIR}/rwbuffer.cc"
    "${CMAKE_CURRENT_LIST_DIR}/splash.cc"
    "${CMAKE_CURRENT_LIST_DIR}/strtools.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vdef.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world.cc")
target_include_directories(shared PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(shared PUBLIC emhash enet entt parson physfs)
target_link_libraries(shared PUBLIC spdlog FNL stb thread_pool)

configure_file("${CMAKE_CURRENT_LIST_DIR}/cmake.hh.in" "${CMAKE_CURRENT_LIST_DIR}/cmake.hh")

if(WIN32)
    # https://github.com/voxelius/voxelius/issues/2#issuecomment-1757782477
    target_compile_definitions(shared PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(shared PUBLIC NOMINMAX)
endif()

if(MSVC)
    # https://github.com/voxelius/voxelius/issues/2#issuecomment-1757782477
    target_compile_options(shared PUBLIC /wd4018)
    target_compile_options(shared PUBLIC /wd4244)
    target_compile_options(shared PUBLIC /wd4267)
endif()
