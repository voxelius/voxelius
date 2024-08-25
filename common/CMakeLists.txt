add_library(common STATIC
    "${CMAKE_CURRENT_LIST_DIR}/cmdline.cc"
    "${CMAKE_CURRENT_LIST_DIR}/config.cc"
    "${CMAKE_CURRENT_LIST_DIR}/crc64.cc"
    "${CMAKE_CURRENT_LIST_DIR}/epoch.cc"
    "${CMAKE_CURRENT_LIST_DIR}/fstools.cc"
    "${CMAKE_CURRENT_LIST_DIR}/image.cc"
    "${CMAKE_CURRENT_LIST_DIR}/message.cc"
    "${CMAKE_CURRENT_LIST_DIR}/strtools.cc")
target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(common PUBLIC mathlib physfs spdlog stb)
