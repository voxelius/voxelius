cmake_minimum_required(VERSION 3.14)
project(libschrift LANGUAGES C VERSION 0.10.2)
add_library(schrift STATIC "${CMAKE_CURRENT_LIST_DIR}/src/schrift.c")
target_include_directories(schrift PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
