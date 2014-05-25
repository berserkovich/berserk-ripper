cmake_minimum_required(VERSION 2.4)

include_directories(${CMAKE_SOURCE_DIR}/minhook/include)

add_library(Ripper SHARED 
            dllmain.cpp 
            RipperDX9.h 
            RipperDX9.cpp)
target_link_libraries(Ripper common minhook)
