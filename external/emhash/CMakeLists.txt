cmake_minimum_required(VERSION 3.15)
project(emhash LANGUAGES CXX)
add_library(emhash INTERFACE)
target_include_directories(emhash INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")
