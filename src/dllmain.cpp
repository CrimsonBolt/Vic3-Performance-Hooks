// vic3hook entry v2: minimal DllMain (no loader-lock work), init on a
// spawned thread, ini hot-reloaded every 5 s so cadence changes apply
// live without re-injection. Crash-fix v2: log line written before every
// install step, immediate flush, and correct file sharing flags.
#include <windows.h>
#include <cstdio>
#include "log.hpp"
#include "hooks.hpp"

extern "C" IMAGE_DOS_HEADER __ImageBase;

static wchar_t g_ini_path[MAX_PATH];
static char g_ini_path_a[MAX_PATH];
static volatile bool g_running = true;

static unsigned read_cadence(const char* name) {
    char key[80];
    snprintf(key, sizeof key, "%s_every_n", name);
    return GetPrivateProfileIntA("hooks", key, 0, g_ini_path_a);
}

static void apply_cadences(bool log_changes) {
    for (int i = 0; i < g_hook_count; ++i) {
        unsigned v = read_cadence(g_hooks[i].name);
        unsigned old = g_hooks[i].every_n;
        if (v != old) {
            hooks_set_cadence(g_hooks[i].name, v);
            if (log_changes)
                log_line("cadence %-42s %u -> %u", g_hooks[i].name, old, v);
        }
    }
}

static DWORD WINAPI init_thread(LPVOID) {
    wchar_t dll_path[MAX_PATH];
    GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), dll_path, MAX_PATH);
    wchar_t* slash = wcsrchr(dll_path, L'\\');
    if (slash) *(slash + 1) = 0;
    lstrcpyW(g_ini_path, dll_path);  lstrcatW(g_ini_path, L"vic3hook.ini");
    WideCharToMultiByte(CP_ACP, 0, g_ini_path, -1, g_ini_path_a, MAX_PATH, nullptr, nullptr);
    wchar_t log_path[MAX_PATH];
    lstrcpyW(log_path, dll_path);    lstrcatW(log_path, L"vic3hook.log");
    log_open(log_path);
    log_line("vic3hook init v2 (%d-hook build " __DATE__ " " __TIME__ ")", g_hook_count);

    HMODULE game = GetModuleHandleW(L"victoria3.exe");
    if (!game) {
        log_line("victoria3.exe not loaded - wrong process?");
        return 0;
    }
    log_line("game base %p", (void*)game);

    // First read of cadences (before install so each target installs with
    // its intended cadence; 0 = disabled = no detour).
    apply_cadences(false);

    log_line("installing hooks (prologue-verified)...");
    hooks_install_all((void*)game);
    log_line("vic3hook ready - ini hot-reloads every 5 s");

    // Hot-reload loop: edit vic3hook.ini, changes apply within 5 s.
    while (g_running) {
        Sleep(5000);
        if (!g_running) break;
        apply_cadences(true);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        HANDLE t = CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr);
        if (t) CloseHandle(t);
    } else if (reason == DLL_PROCESS_DETACH) {
        g_running = false;
        hooks_shutdown();
    }
    return TRUE;
}
