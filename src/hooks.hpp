#pragma once
#include <cstdint>
#include <windows.h>

// One cadence-throttled tick task. `every_n` semantics:
//   0 = hook not installed (task runs every call, vanilla)
//   1 = hook installed but passthrough (call original every call; measuring)
//   N = run original on every Nth call, skip otherwise
struct HookDef {
    const char* name;
    uint32_t rva;              // target VA - preferred image base
    const uint8_t* expect;     // 16 prologue bytes recorded from 1.14.5
    unsigned expect_len;
    void* detour_fn;           // our detour: void (__fastcall*)(void*, void*, void*, void*)
                               // (4 register args forwarded — see hooks.cpp)
    void* original_fn;         // trampoline (set at install)
    uint64_t calls;            // total calls seen
    uint64_t skipped;          // calls we skipped
    unsigned every_n;          // from ini
    bool installed;
};

extern HookDef g_hooks[];
extern const int g_hook_count;

bool hooks_install_all(void* game_base);
// Install a single hook by index (hot-reload path: enables a hook that was
// every_n=0 at injection). Prologue-verified like the bulk installer.
bool hooks_install_one(int index, void* game_base);
void hooks_set_cadence(const char* name, unsigned n);
void hooks_shutdown();
