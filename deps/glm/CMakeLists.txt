cmake_minimum_required(VERSION 3.14)
project(GLM VERSION 1.0.1 LANGUAGES CXX)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")
