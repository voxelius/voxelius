cmake_minimum_required(VERSION 3.14)
project(glad LANGUAGES C VERSION 2.0.0)
add_library(glad STATIC "${CMAKE_CURRENT_LIST_DIR}/src/gl.c")
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
