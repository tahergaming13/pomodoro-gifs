#pragma once

#include <string>

// M5 — Settings persisted to settings.ini (next to the .exe) ---------------
struct PomodoroSettings {
    int  workMin      = 25;
    int  shortMin     = 5;
    int  longMin      = 15;
    int  cycles       = 4;
    bool sound        = true;
    bool alwaysOnTop  = true;
    bool mini         = true;  // start as compact floating widget
    int  winX = 0, winY = 0;
    bool hasPos = false;

    bool load(const std::string& path);
    bool save(const std::string& path) const;
    void resetDefaults();
};

// <exe-dir>\settings.ini
std::string DefaultSettingsPath();
std::string ExeDirectory();
