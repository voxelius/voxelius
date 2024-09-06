cmake_minimum_required(VERSION 3.15)
project(parson LANGUAGES C VERSION 1.5.2)
add_library(parson STATIC "${CMAKE_CURRENT_LIST_DIR}/src/parson.c")
target_include_directories(parson PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
