// vic3hook cadence hooks. Targets are tick-task run bodies (vtable slot 7 /
// Group B prepare-is-run) resolved by the RTTI walk in H35 + H46 sessions
// and scripts/resolve_hook_targets2.py (round 2), valid for victoria3.exe
// 1.14.5 (rev 9f8281a3) ONLY. Every target's first bytes are verified
// against the recorded prologue before a hook is installed; on any mismatch
// the hook is disabled, never blind-patched. Detours forward ALL four
// register args defensively (crash fix v2: the first build truncated to 2
// args and dropped r8/r9). Install steps are logged BEFORE execution so a
// crash leaves a trace.
#include "hooks.hpp"
#include "log.hpp"
#include <MinHook.h>
#include <windows.h>
#include <cstring>

namespace {

typedef void (__fastcall* execute_fn4_t)(void* self, void* ctx, void* a3, void* a4);

struct alignas(16) Prologue { uint8_t b[16]; };

// Round 1 (sessions H35/H46, live since 2026-10-04) - the 6 proven hooks.
// Round-2 targets (UpdatePopGrowth 0x13a3020, ProcessEconomicMigration
// 0x11b0c00, UpdateAICountryGoalsAdjustments 0x1576300, UpdateStatePopNeedCache
// 0x13b7e70, UpdateStateTrades 0x13b7bd0, UpdateWorldMarket 0xedf260) were
// resolved via resolve_hook_targets2.py but REMOVED from the DLL on
// 2026-10-14 pending in-game verification; re-add from this record or by
// re-running the resolver.
constexpr Prologue P_NAVY       = {{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48}};
constexpr Prologue P_BUILDSP2   = {{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48}};
constexpr Prologue P_UPDATEAI   = {{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57}};
constexpr Prologue H_AUTONOMOUS = {{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57}};
constexpr Prologue P_SPENDVAR   = {{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x7c,0x24,0x20,0x55}};
constexpr Prologue P_RECALCMOD  = {{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x20,0x57}};

constexpr int N_HOOKS = 6;
uint64_t g_cnt[N_HOOKS] = {};
volatile unsigned g_every[N_HOOKS] = {};

#define DEF_DETOUR(idx) \
    void __fastcall detour_h##idx(void* self, void* ctx, void* a3, void* a4) { \
        unsigned n = g_every[idx]; \
        if (n > 1 && ++g_cnt[idx] % n) { ++g_hooks[idx].skipped; return; } \
        ++g_hooks[idx].calls; \
        reinterpret_cast<execute_fn4_t>(g_hooks[idx].original_fn)(self, ctx, a3, a4); \
    }

DEF_DETOUR(0)  DEF_DETOUR(1)  DEF_DETOUR(2)  DEF_DETOUR(3)
DEF_DETOUR(4)  DEF_DETOUR(5)

} // namespace

HookDef g_hooks[] = {
    {"PrepareNavyAIData",                     0x0157d990, P_NAVY.b,       16, (void*)&detour_h0,  nullptr, 0, 0, 0, false},
    {"UpdateBuildingSpendingPriorities_Stage2",0x0157cbd0, P_BUILDSP2.b,  16, (void*)&detour_h1,  nullptr, 0, 0, 0, false},
    {"UpdateAI",                              0x01575530, P_UPDATEAI.b,   16, (void*)&detour_h2,  nullptr, 0, 0, 0, false},
    {"HandleAutonomousInvestment",            0x00f6e590, H_AUTONOMOUS.b, 16, (void*)&detour_h3,  nullptr, 0, 0, 0, false},
    {"UpdateSpendingVariables",               0x0157aef0, P_SPENDVAR.b,   16, (void*)&detour_h4,  nullptr, 0, 0, 0, false},
    {"RecalculateModifierNodes",              0x007da740, P_RECALCMOD.b,  16, (void*)&detour_h5,  nullptr, 0, 0, 0, false},
};
const int g_hook_count = sizeof(g_hooks) / sizeof(g_hooks[0]);

static_assert(g_hook_count == N_HOOKS, "detour index arrays must match hook table");

void hooks_set_cadence(const char* name, unsigned n) {
    for (int i = 0; i < g_hook_count; ++i) {
        if (strcmp(g_hooks[i].name, name) == 0) {
            g_hooks[i].every_n = n;
            g_every[i] = n;
            return;
        }
    }
}

bool hooks_install_all(void* game_base) {
    const uintptr_t base = reinterpret_cast<uintptr_t>(game_base);
    if (MH_Initialize() != MH_OK) {
        log_line("MH_Initialize failed");
        return false;
    }
    log_line("MH_Initialize ok");
    // Pacing (2026-10-14): two live sessions deadlocked inside MH_EnableHook
    // when hooks were enabled back-to-back (each enable suspends/resumes ALL
    // game threads; 12 cycles in ~350 ms hit the race at hooks 6 and 10).
    // Gaps between enables let the game's threads quiesce.
    Sleep(500);
    int installed = 0;
    for (int i = 0; i < g_hook_count; ++i) {
        HookDef& h = g_hooks[i];
        if (h.every_n == 0) {
            log_line("hook %d/%d %-42s disabled (every_n=0)", i + 1, g_hook_count, h.name);
            continue;
        }
        void* target = reinterpret_cast<void*>(base + h.rva);
        log_line("hook %d/%d %-42s verifying prologue at %p...", i + 1, g_hook_count, h.name, target);
        if (memcmp(target, h.expect, h.expect_len) != 0) {
            uint8_t got[16];
            memcpy(got, target, 16);
            log_line("hook %-42s DISABLED: prologue mismatch (expected %02x%02x%02x%02x, got %02x%02x%02x%02x)",
                     h.name, h.expect[0], h.expect[1], h.expect[2], h.expect[3],
                     got[0], got[1], got[2], got[3]);
            continue;
        }
        log_line("hook %d/%d %-42s creating...", i + 1, g_hook_count, h.name);
        if (MH_CreateHook(target, h.detour_fn, &h.original_fn) != MH_OK) {
            log_line("hook %-42s FAILED: MH_CreateHook", h.name);
            continue;
        }
        log_line("hook %d/%d %-42s enabling...", i + 1, g_hook_count, h.name);
        if (MH_EnableHook(target) != MH_OK) {
            log_line("hook %-42s FAILED: MH_EnableHook", h.name);
            continue;
        }
        h.installed = true;
        ++installed;
        log_line("hook %-42s INSTALLED at %p, cadence 1/%u", h.name, target, h.every_n);
        Sleep(250);
    }
    log_line("hooks installed: %d/%d", installed, g_hook_count);
    return installed > 0;
}

void hooks_shutdown() {
    for (int i = 0; i < g_hook_count; ++i) {
        HookDef& h = g_hooks[i];
        if (h.installed) {
            void* target = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(GetModuleHandleW(L"victoria3.exe")) + h.rva);
            MH_DisableHook(target);
            MH_RemoveHook(target);
            log_line("hook %-42s removed (calls=%llu skipped=%llu)",
                     h.name, (unsigned long long)h.calls, (unsigned long long)h.skipped);
            h.installed = false;
        }
    }
    MH_Uninitialize();
}
