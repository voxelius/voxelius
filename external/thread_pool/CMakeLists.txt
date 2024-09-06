cmake_minimum_required(VERSION 3.15)
project(thread_pool LANGUAGES CXX VERSION 2.0.0)
add_library(thread_pool INTERFACE)
target_include_directories(thread_pool INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")
