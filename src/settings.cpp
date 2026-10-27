#include "settings.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdio>
#include <fstream>
#include <sstream>

static std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string ExeDirectory() {
    char buf[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string p(buf, n ? n : 0);
    size_t pos = p.find_last_of("\\/");
    if (pos == std::string::npos)
        return ".";
    return p.substr(0, pos);
}

std::string DefaultSettingsPath() {
    return ExeDirectory() + "\\settings.ini";
}

void PomodoroSettings::resetDefaults() {
    workMin = 25; shortMin = 5; longMin = 15; cycles = 4;
    sound = true; alwaysOnTop = true; mini = true;
}

bool PomodoroSettings::load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        return false;
    std::string line;
    auto toInt = [](const std::string& v, int fallback) {
        try { return std::stoi(v); } catch (...) { return fallback; }
    };
    while (std::getline(f, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[')
            continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        std::string key = Trim(line.substr(0, eq));
        std::string val = Trim(line.substr(eq + 1));
        if (key == "work_min")        workMin  = toInt(val, workMin);
        else if (key == "short_min")  shortMin = toInt(val, shortMin);
        else if (key == "long_min")   longMin  = toInt(val, longMin);
        else if (key == "cycles")     cycles   = toInt(val, cycles);
        else if (key == "sound")      sound    = (val == "1" || val == "true");
        else if (key == "always_on_top") alwaysOnTop = (val == "1" || val == "true");
        else if (key == "mini")       mini     = (val == "1" || val == "true");
        else if (key == "win_x") { winX = toInt(val, winX); hasPos = true; }
        else if (key == "win_y") { winY = toInt(val, winY); hasPos = true; }
    }
    // Clamp to sane ranges (same ranges as the settings sliders).
    if (workMin < 1)  workMin = 1;
    if (workMin > 60) workMin = 60;
    if (shortMin < 1)  shortMin = 1;
    if (shortMin > 30) shortMin = 30;
    if (longMin < 1)  longMin = 1;
    if (longMin > 45) longMin = 45;
    if (cycles < 2) cycles = 2;
    if (cycles > 8) cycles = 8;
    return true;
}

bool PomodoroSettings::save(const std::string& path) const {
    std::ofstream f(path, std::ios::trunc);
    if (!f.is_open())
        return false;
    f << "[Pomodoro]\n"
      << "work_min="  << workMin  << "\n"
      << "short_min=" << shortMin << "\n"
      << "long_min="  << longMin  << "\n"
      << "cycles="    << cycles   << "\n"
      << "sound="         << (sound ? 1 : 0) << "\n"
      << "always_on_top=" << (alwaysOnTop ? 1 : 0) << "\n"
      << "mini="          << (mini ? 1 : 0) << "\n"
      << "win_x=" << winX << "\n"
      << "win_y=" << winY << "\n";
    return true;
}
