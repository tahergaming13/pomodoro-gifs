#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// M6 — Global hotkey: Ctrl+Alt+P -> start/pause -----------------------------
inline constexpr int POM_HOTKEY_ID = 1;

bool RegisterPomodoroHotkey(HWND hwnd);  // false if already taken
void UnregisterPomodoroHotkey(HWND hwnd);
