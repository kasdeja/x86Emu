cmake_minimum_required(VERSION 3.10)
project(x86Emu CXX)

add_definitions(-O3)
add_definitions(-ffast-math)
add_definitions(-ftree-vectorize)
add_definitions(-ggdb)
add_definitions(-march=native)

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    set(SDL2_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/win-x64/SDL2-2.32.8/x86_64-w64-mingw32/include)
    execute_process(COMMAND ${PROJECT_SOURCE_DIR}/win-x64/SDL2-2.32.8/x86_64-w64-mingw32/bin/sdl2-config --static-libs OUTPUT_VARIABLE SDL2_LIBRARY OUTPUT_STRIP_TRAILING_WHITESPACE)
    include_directories(${SDL2_INCLUDE_DIR})
endif()

add_subdirectory(src)
