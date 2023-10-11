cmake_minimum_required(VERSION 3.14)
project(GLM VERSION 0.9.9.9 LANGUAGES CXX)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")
