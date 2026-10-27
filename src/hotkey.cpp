#include "hotkey.h"

bool RegisterPomodoroHotkey(HWND hwnd) {
    // MOD_NOREPEAT (0x4000): no repeat events while the keys are held.
    return RegisterHotKey(hwnd, POM_HOTKEY_ID,
                          MOD_CONTROL | MOD_ALT | MOD_NOREPEAT,
                          'P') != 0;
}

void UnregisterPomodoroHotkey(HWND hwnd) {
    UnregisterHotKey(hwnd, POM_HOTKEY_ID);
}
