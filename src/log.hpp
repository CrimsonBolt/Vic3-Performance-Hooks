#pragma once
#include <windows.h>

void log_open(const wchar_t* path);
void log_line(const char* fmt, ...);
