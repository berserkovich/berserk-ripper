cmake_minimum_required(VERSION 2.4)

add_executable(BerserkRipper WIN32
        Ripper.cpp
        Ripper.h
        RipperD3D9.cpp
        WinMain.cpp)
target_link_libraries(BerserkRipper common libinject)