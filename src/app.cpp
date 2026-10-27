#include "app.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <SDL_syswm.h>
#include <GL/gl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "hotkey.h"
#include "ui.h"

#include <thread>

// ---------------------------------------------------------------------------
bool App::init() {
    settingsPath = DefaultSettingsPath();
    settings.load(settingsPath);  // missing file -> defaults, fine
    assetDir = ExeDirectory() + "\\assets";
    syncConfigToTimer();

    if (!initSDL())
        return false;
    if (!initGL()) {
        shutdown();
        return false;
    }
    initImGui();
    loadGifs();

    timer.applyConfig(config);
    timer.reset();  // fresh WORK phase, paused until user presses Start
    timer.onPhaseEnd = [this](PomState s) { onPhaseChanged(s); };

    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);  // needed for WM_HOTKEY (M6)
    lastTick_ = SDL_GetTicks64();
    running = true;
    initialized_ = true;
    return true;
}

// M1 — SDL2 + OpenGL window: borderless, always-on-top -----------------------
bool App::initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
        return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    int x = SDL_WINDOWPOS_CENTERED, y = SDL_WINDOWPOS_CENTERED;
    expanded = !settings.mini;  // mini floating widget is the default view
    winW_ = expanded ? WIN_W : MINI_W;
    winH_ = expanded ? WIN_H : MINI_H;
    if (settings.hasPos) {
        x = settings.winX;
        y = settings.winY;
    } else if (!expanded) {
        // First run: park the mini widget in the bottom-right corner,
        // above the taskbar.
        SDL_Rect usable;
        if (SDL_GetDisplayUsableBounds(0, &usable) == 0) {
            x = usable.x + usable.w - winW_ - 12;
            y = usable.y + usable.h - winH_ - 12;
        }
    }

    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS |
                   SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_ALWAYS_ON_TOP;
    window = SDL_CreateWindow("Pomodoro", x, y, winW_, winH_, flags);
    if (!window)
        return false;

    // Clamp fully inside the usable area (e.g. mode changed since save).
    {
        SDL_Rect usable;
        if (SDL_GetDisplayUsableBounds(0, &usable) == 0) {
            int cx = 0, cy = 0;
            SDL_GetWindowPosition(window, &cx, &cy);
            if (cx + winW_ > usable.x + usable.w)
                cx = usable.x + usable.w - winW_;
            if (cy + winH_ > usable.y + usable.h)
                cy = usable.y + usable.h - winH_;
            if (cx < usable.x) cx = usable.x;
            if (cy < usable.y) cy = usable.y;
            SDL_SetWindowPosition(window, cx, cy);
        }
    }

    // Win32 HWND for topmost / layered / hotkey APIs (M6).
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(window, &info))
        return false;
    hwnd = info.info.win.window;

    applyWindowFlags();

    // M6 — Ctrl+Alt+P -> start/pause (global, works while unfocused).
    hotkeyActive = RegisterPomodoroHotkey(hwnd);
    return true;
}

void App::applyWindowFlags() {
    if (hwnd) {
        // Always floating: topmost unless the user disabled it in settings.
        SetWindowPos(hwnd,
                     settings.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        // Transparent background: pure black becomes see-through, so the
        // UI style must avoid pure black (see initImGui color lift).
        LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    }
    if (window)
        SDL_SetWindowAlwaysOnTop(window,
                                 settings.alwaysOnTop ? SDL_TRUE : SDL_FALSE);
}

bool App::initGL() {
    glContext = SDL_GL_CreateContext(window);
    if (!glContext)
        return false;
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1);  // vsync
    return true;
}

void App::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // no imgui.ini clutter; we use settings.ini
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    // Lift pure blacks: with the LWA_COLORKEY transparency trick, exact
    // black pixels become transparent holes in the window.
    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        ImVec4& c = style.Colors[i];
        if (c.x == 0.0f && c.y == 0.0f && c.z == 0.0f)
            c = ImVec4(0.015f, 0.016f, 0.022f, c.w);
    }
    style.WindowRounding    = 18.0f;
    style.ChildRounding     = 12.0f;
    style.FrameRounding     = 10.0f;
    style.GrabRounding      = 8.0f;
    style.PopupRounding     = 10.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.WindowPadding     = ImVec2(14, 12);
    style.ItemSpacing       = ImVec2(8, 8);

    // Nicer typography via Segoe UI (ships with Windows); fall back to the
    // embedded default font if unavailable.
    const char* segoe = "C:\\Windows\\Fonts\\segoeui.ttf";
    DWORD attrs = GetFileAttributesA(segoe);
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        fontNormal = io.Fonts->AddFontFromFileTTF(segoe, 16.0f);
        fontBig    = io.Fonts->AddFontFromFileTTF(segoe, 46.0f);
    }
    if (!fontNormal)
        fontNormal = io.Fonts->AddFontDefault();
    if (!fontBig)
        fontBig = fontNormal;

    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 130");
}

void App::loadGifs() {
    gifWork.load(assetDir + "\\rocket.gif");   // work session
    gifShort.load(assetDir + "\\clouds.gif");  // short break
    gifLong.load(assetDir + "\\beach.gif");    // long break
    currentGif = &gifWork;
}

void App::syncConfigToTimer() {
    config.workMin  = settings.workMin;
    config.shortMin = settings.shortMin;
    config.longMin  = settings.longMin;
    config.cycles   = settings.cycles;
    timer.applyConfig(config);
}

void App::saveSettings() {
    if (window) {
        int x = 0, y = 0;
        SDL_GetWindowPosition(window, &x, &y);
        settings.winX = x;
        settings.winY = y;
        settings.hasPos = true;
    }
    settings.save(settingsPath);
}

// M2 — Beep on session end (async so the UI never freezes) -------------------
void App::selectGifForState() {
    // Mini widget AND full view both show the phase-appropriate GIF:
    // WORK -> rocket, SHORT_BREAK -> clouds, LONG_BREAK -> beach.
    switch (timer.state()) {
        case PomState::WORK:        currentGif = &gifWork;  break;
        case PomState::SHORT_BREAK: currentGif = &gifShort; break;
        case PomState::LONG_BREAK:  currentGif = &gifLong;  break;
    }
    if (currentGif)
        currentGif->reset();
}

void App::setExpanded(bool e) {
    if (expanded == e || !window)
        return;
    expanded = e;
    showSettings = false;
    int nw = e ? WIN_W : MINI_W;
    int nh = e ? WIN_H : MINI_H;
    // Keep the bottom-right corner pinned so a corner widget expands
    // in place instead of jumping across the screen.
    int x = 0, y = 0;
    SDL_GetWindowPosition(window, &x, &y);
    SDL_SetWindowSize(window, nw, nh);
    SDL_SetWindowPosition(window, x + winW_ - nw, y + winH_ - nh);
    winW_ = nw;
    winH_ = nh;
    selectGifForState();
    settings.mini = !e;
    saveSettings();
}

void App::onPhaseChanged(PomState newState) {
    selectGifForState();

    if (!settings.sound)
        return;
    std::thread([newState]() {
        if (newState == PomState::WORK) {
            Beep(660, 180);  Sleep(120);
            Beep(880, 180);  Sleep(120);
            Beep(990, 320);
        } else if (newState == PomState::SHORT_BREAK) {
            Beep(880, 250);  Sleep(150);
            Beep(660, 350);
        } else {
            Beep(523, 200);  Sleep(130);
            Beep(659, 200);  Sleep(130);
            Beep(784, 400);
        }
    }).detach();
}

// ---------------------------------------------------------------------------
void App::processEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL2_ProcessEvent(&e);
        if (e.type == SDL_QUIT) {
            running = false;
        } else if (e.type == SDL_SYSWMEVENT) {
            const SDL_SysWMmsg* m = e.syswm.msg;
            // M6 — global hotkey pump: Ctrl+Alt+P -> start/pause.
            if (m && m->msg.win.msg == WM_HOTKEY &&
                m->msg.win.wParam == static_cast<WPARAM>(POM_HOTKEY_ID)) {
                timer.togglePause();
            }
        }
    }
}

void App::renderFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    RenderUI(*this);

    ImGui::Render();
    glViewport(0, 0, winW_, winH_);
    // Pure black clear color -> transparent via LWA_COLORKEY.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

int App::run() {
    while (running) {
        processEvents();
        // M2 — delta tick, no threads.
        uint64_t now = SDL_GetTicks64();
        uint64_t delta = (now >= lastTick_) ? (now - lastTick_) : 0;
        lastTick_ = now;
        if (delta > 250)
            delta = 250;  // clamp stalls (e.g. window dragged by OS)
        timer.tick(delta);
        if (currentGif)
            currentGif->update(delta);
        renderFrame();
    }
    saveSettings();
    return 0;
}

void App::shutdown() {
    if (shutdownDone_)
        return;
    shutdownDone_ = true;
    if (hwnd)
        UnregisterPomodoroHotkey(hwnd);  // M6 cleanup
    hwnd = nullptr;
    if (initialized_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
    gifWork.clear();
    gifShort.clear();
    gifLong.clear();
    currentGif = nullptr;
    if (glContext) {
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}
