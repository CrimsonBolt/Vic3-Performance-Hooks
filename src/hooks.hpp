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
    uint32_t rva_alt;          // second supported game build's address
    const uint8_t* expect_alt; // ...and its prologue (0 = single-build hook)
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
// Optional sidecar target table ("vic3hook.targets" next to the DLL):
// TSV rows "name<TAB>rva<TAB>prologue_hex32" add install candidates for
// ANY game version without rebuilding. Called before hooks_install_all.
bool hooks_load_targets_sidecar(const wchar_t* path);
// Install a single hook by index (hot-reload path: enables a hook that was
// every_n=0 at injection). Prologue-verified like the bulk installer.
bool hooks_install_one(int index, void* game_base);
void hooks_set_cadence(const char* name, unsigned n);
// Scheduled resync: every Nth day (tick), all throttled tasks run at full
// fidelity regardless of their cadence. Validated 2026-10-15: a few
// full-fidelity days un-stuck frozen wars that had persisted for decades.
// 0 disables the resync.
void hooks_set_vanilla_day_period(unsigned days);
// 0 = write GameStateTick.RenderFrameIfNeeded to 0 in the game image (same
// effect as launching with -disable_renderframeifneeded: sub-tick rendering
// is sacrificed for tick speed; sim results unaffected). Nonzero = leave the
// game's default (1) alone. Bounds-checked write, logged.
bool hooks_apply_render_frame_option(void* game_base, int enabled);
void hooks_shutdown();
