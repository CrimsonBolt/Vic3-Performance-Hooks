// vic3hook logging - CreateFileW with FILE_SHARE_READ so live tailing works;
// every line flushed immediately (a crash must never lose the trace).
#include "log.hpp"
#include <windows.h>
#include <cstdio>
#include <cstdarg>

static HANDLE g_log = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION g_cs;
static bool g_cs_init = false;

void log_open(const wchar_t* path) {
    if (!g_cs_init) {
        InitializeCriticalSection(&g_cs);
        g_cs_init = true;
    }
    EnterCriticalSection(&g_cs);
    if (g_log == INVALID_HANDLE_VALUE) {
        g_log = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    }
    LeaveCriticalSection(&g_cs);
}

void log_line(const char* fmt, ...) {
    if (g_log == INVALID_HANDLE_VALUE) return;
    char buf[1024];
    SYSTEMTIME st;
    GetLocalTime(&st);
    int n = snprintf(buf, sizeof buf, "[%02d:%02d:%02d.%03d] ",
                     st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    va_list args;
    va_start(args, fmt);
    int m = _vsnprintf_s(buf + n, sizeof buf - n, _TRUNCATE, fmt, args);
    va_end(args);
    if (m < 0) m = 0;
    n += m;
    buf[n++] = '\r'; buf[n++] = '\n';
    EnterCriticalSection(&g_cs);
    DWORD written = 0;
    WriteFile(g_log, buf, (DWORD)n, &written, nullptr);
    FlushFileBuffers(g_log);
    LeaveCriticalSection(&g_cs);
}
