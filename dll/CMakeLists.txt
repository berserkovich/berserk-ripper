cmake_minimum_required(VERSION 2.4)

include_directories(${CMAKE_SOURCE_DIR}/minhook/include)

add_library(RipperDX9 SHARED 
            dllmain.cpp 
            RipperDX9.h 
            RipperDX9.cpp)
target_link_libraries(RipperDX9 minhook)
