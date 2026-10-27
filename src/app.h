#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>

#include <SDL.h>

#include "imgui.h"
#include "timer.h"
#include "settings.h"
#include "gif_player.h"

// Application: owns the SDL window, GL context, timer, GIFs and settings.
// M1 bare window + M6 hotkey + M7 single-exe live here.
class App {
public:
    static constexpr int WIN_W = 320;      // full view
    static constexpr int WIN_H = 460;
    static constexpr int MINI_W = 190;     // compact floating widget
    static constexpr int MINI_H = 232;

    App() = default;
    ~App() { shutdown(); }
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    bool init();
    int  run();   // main loop; returns process exit code
    void shutdown();

    void onPhaseChanged(PomState newState);
    void syncConfigToTimer();
    void applyWindowFlags();
    void saveSettings();
    void setExpanded(bool e);  // toggle full view <-> mini widget
    void selectGifForState();

    // --- state exposed to the UI layer ---
    SDL_Window*    window    = nullptr;
    SDL_GLContext  glContext = nullptr;
    HWND           hwnd      = nullptr;

    PomodoroTimer   timer;
    PomodoroConfig  config;
    PomodoroSettings settings;

    GifPlayer gifWork, gifShort, gifLong;
    GifPlayer* currentGif = nullptr;

    bool running      = false;
    bool showSettings = false;
    bool hotkeyActive = false;
    bool dragArmed    = false;
    bool expanded     = false;  // false = mini widget (default)
    bool gifHovered   = false;  // gif image hovered this frame (drag zone)
    ImVec2 downPos    = ImVec2(0, 0);  // press point for click detection
    bool   downValid  = false;

    struct ImFont* fontNormal = nullptr;
    struct ImFont* fontBig    = nullptr;

    std::string assetDir;
    std::string settingsPath;

private:
    bool initSDL();
    bool initGL();
    void initImGui();
    void loadGifs();
    void processEvents();
    void renderFrame();

    uint64_t lastTick_      = 0;
    bool     initialized_   = false;
    bool     shutdownDone_  = false;
    int      winW_ = MINI_W;  // current window size (mini or full)
    int      winH_ = MINI_H;
};
