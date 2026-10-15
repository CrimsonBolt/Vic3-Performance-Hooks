# vic3hook - MinHook cadence-throttling DLL for Victoria 3 (1.14.5).
# Personal use only. Skips selected every-tick engine tasks on N-1 of
# every N ticks to reclaim sim time; deterministic (same skips on every
# client) but NOT bit-identical to vanilla — single-player focused.
cmake_minimum_required(VERSION 3.21)
project(vic3hook C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "vic3hook must be built 64-bit: configure with 'cmake -A x64'.")
endif()

file(GLOB MINHOOK_SOURCES
    third_party/MinHook/src/*.c
    third_party/MinHook/src/hde/*.c)

add_library(vic3hook SHARED
    src/dllmain.cpp
    src/log.cpp
    src/hooks.cpp
    ${MINHOOK_SOURCES})

target_include_directories(vic3hook PRIVATE
    src
    third_party/MinHook/include
    third_party/MinHook/src)

set_property(TARGET vic3hook PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
