cmake_minimum_required(VERSION 2.4)

include_directories(${CMAKE_SOURCE_DIR}/minhook/include)

add_library(Ripper SHARED 
            dllmain.cpp 
            Hooks.cpp
            Hooks.h
            InputHooks.cpp
            InputHooks.h
            D3D9Hooks.cpp
            D3D9Hooks.h)
target_link_libraries(Ripper common minhook)
