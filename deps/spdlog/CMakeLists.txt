cmake_minimum_required(VERSION 3.14)
project(spdlog VERSION 1.11.0 LANGUAGES CXX)
add_library(spdlog STATIC
    "${CMAKE_CURRENT_LIST_DIR}/src/async.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/bundled_fmtlib_format.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cfg.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/color_sinks.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/file_sinks.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/spdlog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/stdout_sinks.cpp")
target_compile_definitions(spdlog PUBLIC SPDLOG_COMPILED_LIB=1)
target_include_directories(spdlog PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
find_package(Threads REQUIRED)
target_link_libraries(spdlog PUBLIC Threads::Threads)
