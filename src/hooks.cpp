// vic3hook cadence hooks. Targets are tick-task run bodies (vtable slot 7 /
// Group B prepare-is-run) resolved by the RTTI walk recorded in
// scripts/resolve_targets.py. Candidates for victoria3.exe 1.14.5 (rev
// 9f8281a3) and 1.13.11; the first whose prologue verifies installs. Every target is bounds-checked against the loaded image and its first
// bytes verified against the recorded prologue before a hook is installed;
// on any mismatch the hook is disabled, never blind-patched. Detours forward
// ALL four register args defensively (crash fix v2: the first build truncated
// to 2 args and dropped r8/r9) and all targets are verified void-returning
// tick-task Execute/Prepare methods, so the skip path's indeterminate rax is
// never consumed by a caller. Install steps are logged BEFORE execution so a
// crash leaves a trace.
#include "hooks.hpp"
#include "log.hpp"
#include <MinHook.h>
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace {

typedef void (__fastcall* execute_fn4_t)(void* self, void* ctx, void* a3, void* a4);

struct alignas(16) Prologue { uint8_t b[16]; };

// The 6 proven hooks (live-verified 2026-10-04 through 2026-10-14) plus two
// 1.14.6 experiment rows (ProcessEconomicMigration, UpdateAICountryGoals-
// Adjustments — queue/cursor skip-safe per the 2026-10-16 night dive; shipped
// at every_n=0 until live-verified). The other round-2 targets resolved
// 2026-10-14 (UpdatePopGrowth 0x13a3020, UpdateStatePopNeedCache 0x13b7e70,
// UpdateStateTrades 0x13b7bd0, UpdateWorldMarket 0xedf260) stay OUT: pop
// growth is rate-coupled (skips halve birth/death rates) and the market/
// trades/need-cache rebuilds are 28-day recompute tasks where throttling
// mostly buys staleness. Record kept for future re-evaluation.
// NOTE UpdateSpendingVariables: the resolver's vtable slot-7 entry (0x1575af0)
// is an adjustor thunk whose only tail-jump lands on 0x157aef0 — the shipped,
// live-verified target. All call traffic passes through the thunk, so the two
// placements are equivalent; this table hooks the thunk target.
constexpr Prologue P_NAVY       = {{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48}};
constexpr Prologue P_BUILDSP2   = {{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48}};
constexpr Prologue P_UPDATEAI   = {{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57}};
constexpr Prologue H_AUTONOMOUS = {{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57}};
constexpr Prologue P_SPENDVAR   = {{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x7c,0x24,0x20,0x55}};
constexpr Prologue P_RECALCMOD  = {{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x20,0x57}};
// 1.14.6 experiment (night-dive 2026-10-16): ProcessEconomicMigration is a
// queue task (records accumulate; a skipped call delays migration one day,
// nothing is lost) and UpdateAICountryGoalsAdjustments is a sliced-budget
// task with a progress cursor (skip-safe by design). Both ship at every_n=0
// until live-verified. Prologues read from the pristine 1.14.5 exe.
constexpr Prologue P_ECONMIGR   = {{0x48,0x8b,0xc4,0x48,0x81,0xec,0xb8,0x00,0x00,0x00,0x48,0x89,0x58,0x08,0x48,0x89}};
constexpr Prologue P_AIGOALS    = {{0x4c,0x8b,0xdc,0x49,0x89,0x5b,0x08,0x49,0x89,0x73,0x10,0x57,0x48,0x83,0xec,0x60}};

constexpr int N_HOOKS = 8;
uint64_t g_cnt[N_HOOKS] = {};
volatile unsigned g_every[N_HOOKS] = {};
// Scheduled resync: on every day (call) that is a multiple of this period,
// throttled tasks run at full fidelity. All six tasks are called exactly
// once per day by the scheduler, so each detour's call counter IS a day
// counter. Default 7 (one vanilla day per week); 0 = off.
volatile unsigned g_vanilla_day_period = 7;
bool g_mh_ready = false;
int g_build_hint = 0;  // 1 = 1.14.5 candidates matched, 2 = 1.13.11, 0 = none yet

#define DEF_DETOUR(idx) \
    void __fastcall detour_h##idx(void* self, void* ctx, void* a3, void* a4) { \
        unsigned n = g_every[idx]; \
        if (n > 1 && ++g_cnt[idx] % n) { \
            unsigned period = g_vanilla_day_period; \
            if (!(period > 0 && (unsigned)g_cnt[idx] % period == 0)) { \
                ++g_hooks[idx].skipped; \
                return; \
            } \
        } \
        ++g_hooks[idx].calls; \
        reinterpret_cast<execute_fn4_t>(g_hooks[idx].original_fn)(self, ctx, a3, a4); \
    }

DEF_DETOUR(0)  DEF_DETOUR(1)  DEF_DETOUR(2)  DEF_DETOUR(3)
DEF_DETOUR(4)  DEF_DETOUR(5)  DEF_DETOUR(6)  DEF_DETOUR(7)

// A stored RVA is only trustworthy if the whole prologue window lies inside
// the loaded game image AND is committed readable memory. Without this, a
// future (smaller/different-layout) exe turns the prologue check itself into
// an access violation — a crash instead of a fail-closed skip.
bool target_readable(const void* target, size_t len, uintptr_t base) {
    const uint8_t* b = reinterpret_cast<const uint8_t*>(base);
    const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(b);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 0x1000)
        return false;
    const IMAGE_NT_HEADERS* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(b + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return false;
    const uintptr_t image_end = base + nt->OptionalHeader.SizeOfImage;
    const uintptr_t t = reinterpret_cast<uintptr_t>(target);
    if (t < base || t >= image_end || len > image_end - t)
        return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(target, &mbi, sizeof mbi) == 0)
        return false;
    if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
        return false;
    return t + len <= reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
}

// ---- sidecar target table (version-driven packaging) -----------------------
// Rows from vic3hook.targets extend each hook's candidate list at runtime:
// a new game build ships as a data file, never a rebuild. Build ids for
// logging/gating: 1 = built-in 1.14.5, 2 = built-in 1.13.11, 3+ = sidecar.
#define SIDECAR_MAX_PER_HOOK 6
#define SIDECAR_MAX_TOTAL 64
struct SidecarRow { uint32_t rva; uint8_t pro[16]; };
static SidecarRow g_sidecar[8][SIDECAR_MAX_PER_HOOK];
static int g_sidecar_n[8];



bool install_hook_at(int i, uintptr_t base) {
    HookDef& h = g_hooks[i];
    // Each hook carries one candidate per supported game build (primary =
    // 1.14.5, alt = 1.13.11): install on the first whose prologue verifies,
    // fail closed when none does. Keeps the injector's verdict keywords
    // ("prologue mismatch" / "outside the game image") intact.
    uint32_t rvas[2 + SIDECAR_MAX_PER_HOOK];
    const uint8_t* expects[2 + SIDECAR_MAX_PER_HOOK];
    int ncand = 0;
    if (h.rva && h.expect) { rvas[ncand] = h.rva; expects[ncand] = h.expect; ++ncand; }
    if (h.rva_alt && h.expect_alt) { rvas[ncand] = h.rva_alt; expects[ncand] = h.expect_alt; ++ncand; }
    for (int sc = 0; sc < g_sidecar_n[i]; ++sc) { rvas[ncand] = g_sidecar[i][sc].rva; expects[ncand] = g_sidecar[i][sc].pro; ++ncand; }
    void* target = nullptr;
    int build = -1;
    for (int c = 0; c < ncand; ++c) {
        void* cand = reinterpret_cast<void*>(base + rvas[c]);
        log_line("hook %-42s verifying target at %p...", h.name, cand);
        if (!target_readable(cand, h.expect_len, base)) {
            log_line("hook %-42s candidate %d: outside the game image (game updated?)",
                     h.name, c + 1);
            continue;
        }
        if (memcmp(cand, expects[c], h.expect_len) != 0) {
            uint8_t got[16];
            memcpy(got, cand, 16);
            log_line("hook %-42s candidate %d: prologue mismatch (expected %02x%02x%02x%02x, got %02x%02x%02x%02x)",
                     h.name, c + 1, expects[c][0], expects[c][1], expects[c][2], expects[c][3],
                     got[0], got[1], got[2], got[3]);
            continue;
        }
        target = cand;
        build = c;
        break;
    }
    if (!target) {
        log_line("hook %-42s DISABLED: no candidate matched (unsupported game version?)",
                 h.name);
        return false;
    }
    if (MH_CreateHook(target, h.detour_fn, &h.original_fn) != MH_OK) {
        log_line("hook %-42s FAILED: MH_CreateHook", h.name);
        return false;
    }
    // Pacing: MinHook's enable suspends/resumes all game threads; a gap lets
    // them quiesce (two deadlocks at rapid back-to-back enables, 2026-10-14).
    Sleep(250);
    if (MH_EnableHook(target) != MH_OK) {
        MH_RemoveHook(target);  // do not leave a created-but-disabled hook behind
        log_line("hook %-42s FAILED: MH_EnableHook (hook removed)", h.name);
        return false;
    }
    h.installed = true;
    if (g_build_hint == 0)
        g_build_hint = build + 1;
    log_line("hook %-42s INSTALLED at %p (build %d), cadence 1/%u", h.name, target, build + 1, h.every_n);
    return true;
}


} // namespace

bool hooks_load_targets_sidecar(const wchar_t* path) {
    FILE* f = _wfopen(path, L"rb");
    if (!f)
        return false;
    char line[256];
    int rows = 0, bad = 0;
    while (fgets(line, sizeof line, f)) {
if (line[0] == '#' || line[0] == '\n' || line[0] == '\r' || line[0] == 0)
            continue;
        char* t1 = strchr(line, '	'); if (!t1) { ++bad; continue; }
        *t1 = 0;
        char* t2 = strchr(t1 + 1, '	'); if (!t2) { ++bad; continue; }
        *t2 = 0;
        const char* hex = t2 + 1;
        size_t hexlen = strnlen(hex, 40);
        while (hexlen && (hex[hexlen-1] == '\n' || hex[hexlen-1] == '\r')) --hexlen;
        if (hexlen != 32) { ++bad; continue; }
        int idx = -1;
        for (int i = 0; i < g_hook_count; ++i)
            if (strcmp(g_hooks[i].name, line) == 0) { idx = i; break; }
        if (idx < 0 || g_sidecar_n[idx] >= SIDECAR_MAX_PER_HOOK || rows >= SIDECAR_MAX_TOTAL) {
            ++bad; continue;
        }
        SidecarRow& r = g_sidecar[idx][g_sidecar_n[idx]];
        r.rva = (uint32_t)strtoul(t1 + 1, nullptr, 0);
        for (int b = 0; b < 16; ++b) {
            char byte[3] = { hex[b*2], hex[b*2+1], 0 };
            r.pro[b] = (uint8_t)strtoul(byte, nullptr, 16);
        }
        ++g_sidecar_n[idx]; ++rows;
    }
    fclose(f);
    int hooks = 0;
    for (int i = 0; i < g_hook_count; ++i) if (g_sidecar_n[i]) ++hooks;
    log_line("sidecar targets: %d row(s) for %d hook(s) loaded%s",
             rows, hooks, bad ? " (malformed lines skipped)" : "");
    return rows > 0;
}


// 1.13.11 candidate prologue (the only one that differs from its 1.14.5
// counterpart — every other hooked function kept identical bytes between
// builds, just relocated). From scripts/resolve_targets.py on the 1.13.11
// exe (see vic3-re/notes/kit-1.13.11-plan.md).
constexpr Prologue P_BUILDSP2_113 = {{0x4c,0x8b,0xdc,0x53,0x56,0x48,0x81,0xec,0xd8,0x00,0x00,0x00,0x48,0x8b,0x42,0x08}};

HookDef g_hooks[] = {
    {"PrepareNavyAIData",                     0x0157d990, P_NAVY.b,       16, 0x01516af0, P_NAVY.b,       (void*)&detour_h0,  nullptr, 0, 0, 0, false},
    {"UpdateBuildingSpendingPriorities_Stage2",0x0157cbd0, P_BUILDSP2.b,  16, 0x0150fa50, P_BUILDSP2_113.b,(void*)&detour_h1,  nullptr, 0, 0, 0, false},
    {"UpdateAI",                              0x01575530, P_UPDATEAI.b,   16, 0x0150f470, P_UPDATEAI.b,   (void*)&detour_h2,  nullptr, 0, 0, 0, false},
    {"HandleAutonomousInvestment",            0x00f6e590, H_AUTONOMOUS.b, 16, 0x00f168b0, H_AUTONOMOUS.b, (void*)&detour_h3,  nullptr, 0, 0, 0, false},
    {"UpdateSpendingVariables",               0x0157aef0, P_SPENDVAR.b,   16, 0x015142b0, P_SPENDVAR.b,   (void*)&detour_h4,  nullptr, 0, 0, 0, false},
    {"RecalculateModifierNodes",              0x007da740, P_RECALCMOD.b,  16, 0x007c11e0, P_RECALCMOD.b,  (void*)&detour_h5,  nullptr, 0, 0, 0, false},
    {"ProcessEconomicMigration",              0x011b0c00, P_ECONMIGR.b,   16, 0x01152370, P_ECONMIGR.b,   (void*)&detour_h6,  nullptr, 0, 0, 0, false},
    {"UpdateAICountryGoalsAdjustments",       0x01576300, P_AIGOALS.b,    16, 0x015101a0, P_AIGOALS.b,    (void*)&detour_h7,  nullptr, 0, 0, 0, false},
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

void hooks_set_vanilla_day_period(unsigned days) {
    g_vanilla_day_period = days;
}

// GameStateTick.RenderFrameIfNeeded storage byte (victoria3.exe 1.14.5).
// Writing 0 here is identical to launching with -disable_renderframeifneeded
// ("sacrifice sub-tick rendering for tick speed"): it disables the named
// tick-scope timing machinery that feeds the sub-tick render decision.
// Task execution and per-task profiler timing do NOT go through those
// scopes (verified against the pump decompile), and the sim is unaffected —
// only render pacing changes. Bounds-checked like every other write.
bool hooks_apply_render_frame_option(void* game_base, int enabled) {
    if (enabled != 0)
        return true;  // game default (1); we never force it back on
    if (g_build_hint != 1) {  // cvar address is 1.14.5-specific
        log_line("render_frame_if_needed: skipped - game is not the 1.14.5 build (address verified for 1.14.5 only)");
        return false;
    }
    const uintptr_t base = reinterpret_cast<uintptr_t>(game_base);
    uint8_t* p = reinterpret_cast<uint8_t*>(base + 0x051f3e50);
    if (!target_readable(p, 1, base)) {
        log_line("render_frame_if_needed: target byte outside the game image - NOT written");
        return false;
    }
    *p = 0;
    log_line("render_frame_if_needed=0 applied (GameStateTick.RenderFrameIfNeeded -> 0)");
    return true;
}

bool hooks_install_all(void* game_base) {
    const uintptr_t base = reinterpret_cast<uintptr_t>(game_base);
    if (!g_mh_ready) {
        if (MH_Initialize() != MH_OK) {
            log_line("MH_Initialize failed");
            return false;
        }
        g_mh_ready = true;
        log_line("MH_Initialize ok");
    }
    Sleep(500);  // initial pacing before the first thread-suspending enable
    int installed = 0;
    for (int i = 0; i < g_hook_count; ++i) {
        HookDef& h = g_hooks[i];
        if (h.every_n == 0) {
            log_line("hook %d/%d %-42s disabled (every_n=0)", i + 1, g_hook_count, h.name);
            continue;
        }
        if (h.installed) {
            log_line("hook %d/%d %-42s already installed", i + 1, g_hook_count, h.name);
            continue;
        }
        if (install_hook_at(i, base))
            ++installed;
    }
    log_line("hooks installed: %d/%d", installed, g_hook_count);
    return installed > 0;
}

bool hooks_install_one(int index, void* game_base) {
    if (index < 0 || index >= g_hook_count || !g_mh_ready)
        return false;
    if (g_hooks[index].installed)
        return true;
    return install_hook_at(index, reinterpret_cast<uintptr_t>(game_base));
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
    if (g_mh_ready) {
        MH_Uninitialize();
        g_mh_ready = false;
    }
}
