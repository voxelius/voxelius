add_library(shared STATIC
    "${CMAKE_CURRENT_LIST_DIR}/aabb.cc"
    "${CMAKE_CURRENT_LIST_DIR}/cmdline.cc"
    "${CMAKE_CURRENT_LIST_DIR}/config/config.cc"
    "${CMAKE_CURRENT_LIST_DIR}/crc64.cc"
    "${CMAKE_CURRENT_LIST_DIR}/epoch.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/inertial.cc"
    "${CMAKE_CURRENT_LIST_DIR}/ray_dda.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vdef.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vfs.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world.cc")
target_include_directories(shared PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(shared PUBLIC entt glm parson physfs spdlog thread_pool)

if(WIN32)
    # https://github.com/voxelius/voxelius/issues/2#issuecomment-1757782477
    target_compile_definitions(shared PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

if(MSVC)
    # https://github.com/voxelius/voxelius/issues/2#issuecomment-1757782477
    target_compile_options(shared PUBLIC /wd4244)
    target_compile_options(shared PUBLIC /wd4267)
endif()
