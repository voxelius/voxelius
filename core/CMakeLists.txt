add_library(core STATIC
    "${CMAKE_CURRENT_LIST_DIR}/cmdline.cc"
    "${CMAKE_CURRENT_LIST_DIR}/crc64.cc"
    "${CMAKE_CURRENT_LIST_DIR}/epoch.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vfs.cc")
target_include_directories(core PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(core PUBLIC glm physfs spdlog)
