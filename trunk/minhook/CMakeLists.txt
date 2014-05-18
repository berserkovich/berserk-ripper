cmake_minimum_required(VERSION 2.4)
project(minhook)

include_directories(include)

add_library(minhook
            include/MinHook.h
            src/HDE32/hde32.c
            src/HDE32/hde32.h
            src/HDE32/table32.h
            src/HDE64/include/hde64.h
            src/HDE64/src/hde64.c
            src/HDE64/src/table64.h
            src/buffer.cpp
            src/buffer.h
            src/export.cpp
            src/hook.cpp
            src/hook.h
            src/pstdint.h
            src/thread.cpp
            src/thread.h
            src/trampoline.cpp
            src/trampoline.h)