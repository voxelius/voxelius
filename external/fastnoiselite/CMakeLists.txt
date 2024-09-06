cmake_minimum_required(VERSION 3.15)
project(FastNoiseLite LANGUAGES C VERSION 1.1.1)
add_library(FNL STATIC "${CMAKE_CURRENT_LIST_DIR}/src/FastNoiseLite.c")
target_include_directories(FNL PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
