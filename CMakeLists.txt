cmake_minimum_required(VERSION 2.4)
project(berserk-ripper)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(minhook)
add_subdirectory(common)
add_subdirectory(dll)
add_subdirectory(exe)
