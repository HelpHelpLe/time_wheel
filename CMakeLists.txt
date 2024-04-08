cmake_minimum_required(VERSION 3.15)

project(time_wheel)

add_compile_options(--std=c++11)
add_compile_options(-g)
add_compile_options(-Wall)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/)

add_executable(test test.cc)
