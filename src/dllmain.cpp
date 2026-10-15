// vic3hook entry v3: minimal DllMain (no loader-lock work), init on a spawned
// thread, ini hot-reloaded every 5 s so cadence changes apply live without
// re-injection — including ENABLING a hook that was 0/off at injection (the
// reload path installs it, prologue-verified, with the same pacing). Ini
// reading is wide-char end to end so non-ANSI install paths work. Detach is
// loader-lock safe: at process exit nothing is torn down; on a real unload
// the init thread is joined first.
#include <windows.h>
#include <cstdio>
#include "log.hpp"
#include "hooks.hpp"

extern "C" IMAGE_DOS_HEADER __ImageBase;

static wchar_t g_ini_path[1024];
static volatile bool g_running = true;
static uintptr_t g_game_base = 0;
static HANDLE g_init_thread = nullptr;

static unsigned read_cadence(const char* name) {
    char key[80];
    snprintf(key, sizeof key, "%s_every_n", name);
    wchar_t wkey[80];
    MultiByteToWideChar(CP_UTF8, 0, key, -1, wkey, 80);
    return GetPrivateProfileIntW(L"hooks", wkey, 0, g_ini_path);
}

static void apply_cadences(bool log_changes) {
    for (int i = 0; i < g_hook_count; ++i) {
        unsigned v = read_cadence(g_hooks[i].name);
        unsigned old = g_hooks[i].every_n;
        if (v == old)
            continue;
        hooks_set_cadence(g_hooks[i].name, v);
        if (!log_changes)
            continue;
        if (old == 0 && v != 0 && !g_hooks[i].installed) {
            // enabling a hook that was off at injection: install it now
            log_line("cadence %-42s %u -> %u (installing hook)", g_hooks[i].name, old, v);
            if (!hooks_install_one(i, (void*)g_game_base))
                log_line("cadence %-42s NOT APPLIED - hook install failed (see above); task stays vanilla",
                         g_hooks[i].name);
        } else if (v != 0 && !g_hooks[i].installed) {
            log_line("cadence %-42s %u -> %u NOT APPLIED - hook not installed (set 0, then the desired value, or re-inject)",
                     g_hooks[i].name, old, v);
        } else {
            log_line("cadence %-42s %u -> %u", g_hooks[i].name, old, v);
        }
    }
}

static DWORD WINAPI init_thread(LPVOID) {
    wchar_t dll_path[1024];
    GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), dll_path, 1024);
    wchar_t* slash = wcsrchr(dll_path, L'\\');
    if (slash) *(slash + 1) = 0;
    if (wcslen(dll_path) + 14 > 1024) return 0;  // cannot happen with MAX_PATH dirs; guard anyway
    lstrcpyW(g_ini_path, dll_path);  lstrcatW(g_ini_path, L"vic3hook.ini");
    wchar_t log_path[1024];
    lstrcpyW(log_path, dll_path);    lstrcatW(log_path, L"vic3hook.log");
    log_open(log_path);
    log_line("vic3hook init v3 (%d-hook build " __DATE__ " " __TIME__ ")", g_hook_count);

    HMODULE game = GetModuleHandleW(L"victoria3.exe");
    if (!game) {
        log_line("victoria3.exe not loaded - wrong process?");
        return 0;
    }
    g_game_base = reinterpret_cast<uintptr_t>(game);
    log_line("game base %p", (void*)game);

    // First read of cadences (before install so each target installs with
    // its intended cadence; 0 = not installed = vanilla).
    apply_cadences(false);

    log_line("installing hooks (prologue-verified)...");
    if (hooks_install_all((void*)game))
        log_line("vic3hook ready - ini hot-reloads every 5 s");
    else
        log_line("vic3hook ready (NO hooks installed) - see log above; "
                 "ini hot-reload can install hooks once the cause is fixed");

    // Hot-reload loop: edit vic3hook.ini, changes apply within 5 s.
    while (g_running) {
        Sleep(5000);
        if (!g_running) break;
        apply_cadences(true);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        g_init_thread = CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr);
    } else if (reason == DLL_PROCESS_DETACH) {
        g_running = false;
        if (reserved != nullptr)
            return TRUE;  // process exit: no loader-lock work, no MinHook teardown
        if (g_init_thread) {
            WaitForSingleObject(g_init_thread, 3000);
            CloseHandle(g_init_thread);
            g_init_thread = nullptr;
        }
        hooks_shutdown();
    }
    return TRUE;
}
