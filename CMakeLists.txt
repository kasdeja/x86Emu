cmake_minimum_required(VERSION 3.5)
project(x86Emu CXX)

add_definitions(-O3)
add_definitions(-ffast-math)
add_definitions(-ftree-vectorize)
add_definitions(-ggdb)
add_definitions(-march=native)

add_subdirectory(src)
