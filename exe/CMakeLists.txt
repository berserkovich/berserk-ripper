cmake_minimum_required(VERSION 2.4)

add_executable(BerserkRipper WIN32
		WinMain.cpp)
target_link_libraries(BerserkRipper libinject)