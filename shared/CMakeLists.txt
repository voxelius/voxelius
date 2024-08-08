add_library(shared STATIC
    "${CMAKE_CURRENT_LIST_DIR}/entity/transform.cc"
    "${CMAKE_CURRENT_LIST_DIR}/entity/velocity.cc"
    "${CMAKE_CURRENT_LIST_DIR}/math/mat4x4f.cc"
    "${CMAKE_CURRENT_LIST_DIR}/math/vec2f.cc"
    "${CMAKE_CURRENT_LIST_DIR}/math/vec3angles.cc"
    "${CMAKE_CURRENT_LIST_DIR}/math/vec3f.cc"
    "${CMAKE_CURRENT_LIST_DIR}/math/vec4f.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/crc64.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/epoch.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/physfs.cc"
    "${CMAKE_CURRENT_LIST_DIR}/util/string.cc"
    "${CMAKE_CURRENT_LIST_DIR}//ray_dda.cc"
    "${CMAKE_CURRENT_LIST_DIR}/cmdline.cc"
    "${CMAKE_CURRENT_LIST_DIR}/config.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/image.cc"
    "${CMAKE_CURRENT_LIST_DIR}/splash.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vdef.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world.cc")
target_include_directories(shared PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(shared PUBLIC emhash entt parson physfs)
target_link_libraries(shared PUBLIC spdlog FNL stb thread_pool)

configure_file("${CMAKE_CURRENT_LIST_DIR}/cmake.hh.in" "${CMAKE_CURRENT_LIST_DIR}/cmake.hh")

if(WIN32)
    # https://github.com/voxelius/voxelius/issues/2#issuecomment-1757782477
    target_compile_definitions(shared PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

if(MSVC)
    # https://github.com/voxelius/voxelius/issues/2#issuecomment-1757782477
    target_compile_options(shared PUBLIC /wd4018)
    target_compile_options(shared PUBLIC /wd4244)
    target_compile_options(shared PUBLIC /wd4267)

    # This forces MSVC to use UTF-8 for string literals;
    # as usual, MSVC is very "special" and all the string
    # literals (even explicit UTF-32 ones) are invalid when
    # we're dealing with text rendering via char32_t. Too bad!
    target_compile_options(shared PUBLIC /utf-8)
endif()
