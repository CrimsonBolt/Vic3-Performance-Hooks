# vic3hook - MinHook cadence-throttling DLL for Victoria 3 (1.14.5).
# Skips selected every-tick engine tasks on N-1 of every N ticks to reclaim
# sim time; deterministic (same skips on every client) but NOT bit-identical
# to vanilla — single-player focused.
cmake_minimum_required(VERSION 3.21)
project(vic3hook C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "vic3hook must be built 64-bit: configure with 'cmake -A x64'.")
endif()

file(GLOB MINHOOK_SOURCES CONFIGURE_DEPENDS
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

# Assemble the release kit after every build: kit/ holds the runnable folder
# (zip its CONTENTS for a GitHub release). The copied files are gitignored —
# the repo stays source-only.
add_custom_command(TARGET vic3hook POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:vic3hook>
            ${CMAKE_CURRENT_SOURCE_DIR}/kit/vic3hook.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/inject_running.ps1
            ${CMAKE_CURRENT_SOURCE_DIR}/kit/inject_running.ps1
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/MinHook/LICENSE.txt
            ${CMAKE_CURRENT_SOURCE_DIR}/kit/MINHOOK-LICENSE.txt
    COMMENT "Assembling kit/ (zip its contents for a release)")
