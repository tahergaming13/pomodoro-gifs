#include "ui.h"

#include "app.h"
#include "imgui.h"

#include <cfloat>
#include <cstdio>

namespace {

// Accent color per pomodoro state.
ImVec4 AccentFor(PomState s) {
    switch (s) {
        case PomState::WORK:        return ImVec4(0.96f, 0.36f, 0.30f, 1.0f);
        case PomState::SHORT_BREAK: return ImVec4(0.32f, 0.76f, 0.47f, 1.0f);
        case PomState::LONG_BREAK:  return ImVec4(0.38f, 0.62f, 0.96f, 1.0f);
    }
    return ImVec4(0.96f, 0.36f, 0.30f, 1.0f);
}

const char* HintFor(PomState s) {
    switch (s) {
        case PomState::WORK:        return "Focus. One thing at a time.";
        case PomState::SHORT_BREAK: return "Breathe. Stretch. Look away.";
        case PomState::LONG_BREAK:  return "Rest well. You earned it.";
    }
    return "";
}

void FormatTime(char* buf, size_t n, uint64_t ms) {
    uint64_t totalSec = (ms + 999) / 1000;
    std::snprintf(buf, n, "%02llu:%02llu",
                  (unsigned long long)(totalSec / 60),
                  (unsigned long long)(totalSec % 60));
}

void CenteredText(const char* text) {
    float w = ImGui::CalcTextSize(text).x;
    float avail = ImGui::GetContentRegionAvail().x;
    if (w < avail)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - w) * 0.5f);
    ImGui::TextUnformatted(text);
}

// Big countdown drawn straight to the draw list (not an ImGui item, so it
// never blocks window dragging). Includes a soft non-black shadow so the
// time stays readable over light wallpapers (pure black pixels are the
// transparency colorkey and would vanish).
void DrawBigTime(App& app, const char* text, float px, ImU32 col) {
    ImFont* font = app.fontBig ? app.fontBig : ImGui::GetFont();
    ImVec2 size = font->CalcTextSizeA(px, FLT_MAX, 0.0f, text);
    float avail = ImGui::GetContentRegionAvail().x;
    float x = ImGui::GetCursorScreenPos().x;
    if (size.x < avail)
        x += (avail - size.x) * 0.5f;
    float y = ImGui::GetCursorScreenPos().y;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText(font, px, ImVec2(x + 2, y + 2), IM_COL32(30, 32, 40, 200),
                text);
    dl->AddText(font, px, ImVec2(x, y), col, text);
    ImGui::Dummy(ImVec2(avail, size.y));
}

void OsDragWindow(App& app) {
    app.dragArmed = true;
    ReleaseCapture();
    SendMessageW(app.hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
}

}  // namespace

// ---------------------------------------------------------------------------
// Mini widget: floating rocket + time only. Click to enter the full app.
// ---------------------------------------------------------------------------
static void RenderMini(App& app) {
    PomodoroTimer& timer = app.timer;
    const ImVec4 accent = AccentFor(timer.state());

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(
        ImVec2((float)App::MINI_W, (float)App::MINI_H));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground;

    ImGui::Begin("##pomodoro-mini", nullptr, flags);

    // Rocket mascot (transparent background -> floats over the desktop).
    constexpr float GIF_SIZE = 160.0f;
    {
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - GIF_SIZE) * 0.5f);
        if (app.currentGif && app.currentGif->valid()) {
            app.currentGif->draw(GIF_SIZE);
            app.gifHovered = ImGui::IsItemHovered();
        } else {
            ImGui::Dummy(ImVec2(GIF_SIZE, GIF_SIZE));
            app.gifHovered = false;
        }
    }

    // Time below the rocket, tinted by phase; dimmed while paused.
    {
        char buf[16];
        FormatTime(buf, sizeof(buf), timer.remainingMs());
        ImU32 col = ImGui::ColorConvertFloat4ToU32(accent);
        if (timer.paused())
            col = (col & 0x00FFFFFFu) | (0x8Cu << 24);
        DrawBigTime(app, buf, 40.0f, col);
    }

    bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    // Click without dragging -> enter the full app.
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        app.downPos = ImGui::GetMousePos();
        app.downValid = true;
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && app.downValid) {
        app.downValid = false;
        ImVec2 now = ImGui::GetMousePos();
        float dx = now.x - app.downPos.x, dy = now.y - app.downPos.y;
        if (dx * dx + dy * dy < 36.0f && hovered)
            app.setExpanded(true);
    }

    // Click-drag anywhere to move.
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        app.dragArmed = false;
    else if (!app.dragArmed && hovered &&
             ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f)) {
        OsDragWindow(app);
    }

    if (hovered && !ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.0f))
        ImGui::SetTooltip("Click to open\nRight-click for menu - drag to move");

    // Right-click menu.
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##ctx");
    if (ImGui::BeginPopup("##ctx")) {
        if (ImGui::MenuItem("Open"))
            app.setExpanded(true);
        if (ImGui::MenuItem(timer.paused() ? "Start" : "Pause"))
            timer.togglePause();
        if (ImGui::MenuItem("Skip phase"))
            timer.skip();
        if (ImGui::MenuItem("Reset timer"))
            timer.reset();
        ImGui::Separator();
        if (ImGui::MenuItem("Quit"))
            app.running = false;
        ImGui::EndPopup();
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}

// ---------------------------------------------------------------------------
// Full view: panel UI with controls + settings.
// ---------------------------------------------------------------------------
static void RenderFull(App& app) {
    PomodoroTimer& timer = app.timer;
    const ImVec4 accent = AccentFor(timer.state());

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)App::WIN_W, (float)App::WIN_H));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.085f, 0.09f, 0.12f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, accent);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::Begin("##pomodoro-root", nullptr, flags);

    // --- custom title bar ----------------------------------------------------
    ImGui::PushStyleColor(ImGuiCol_Text, accent);
    ImGui::TextUnformatted(timer.stateLabel());
    ImGui::PopStyleColor();
    ImGui::SameLine();
    {
        float bx = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
        const float closeW = 30.0f, minW = 30.0f, setW = 74.0f, gap = 6.0f;
        ImGui::SetCursorPosX(bx - closeW - gap - minW - gap - setW);
        if (ImGui::SmallButton("Settings"))
            app.showSettings = !app.showSettings;
        ImGui::SameLine();
        if (ImGui::SmallButton("-"))
            app.setExpanded(false);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Collapse to mini widget");
        ImGui::SameLine();
        if (ImGui::SmallButton("X"))
            app.running = false;
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator,
                          ImVec4(accent.x, accent.y, accent.z, 0.45f));
    ImGui::Separator();
    ImGui::PopStyleColor();

    // --- GIF ------------------------------------------------------------------
    constexpr float GIF_SIZE = 208.0f;
    {
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - GIF_SIZE) * 0.5f);
        if (app.currentGif && app.currentGif->valid()) {
            app.currentGif->draw(GIF_SIZE);
            app.gifHovered = ImGui::IsItemHovered();
        } else {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            dl->AddRectFilled(p, ImVec2(p.x + GIF_SIZE, p.y + GIF_SIZE),
                              IM_COL32(30, 32, 44, 255), 12.0f);
            ImGui::Dummy(ImVec2(GIF_SIZE, GIF_SIZE));
            app.gifHovered = false;
        }
    }

    // --- big MM:SS countdown ----------------------------------------------------
    {
        char buf[16];
        FormatTime(buf, sizeof(buf), timer.remainingMs());
        DrawBigTime(app, buf, 46.0f, IM_COL32(235, 238, 245, 255));
    }

    // --- progress bar ------------------------------------------------------------
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, accent);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.16f, 0.17f, 0.22f, 1.0f));
    ImGui::ProgressBar((float)timer.progress(), ImVec2(-1, 7), "");
    ImGui::PopStyleColor(2);

    // --- session dots -------------------------------------------------------------
    {
        int total = app.config.cycles > 0 ? app.config.cycles : 4;
        int done = timer.cyclePosition();
        constexpr float R = 5.5f, STEP = 19.0f;
        float rowW = (total - 1) * STEP + R * 2;
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 base(ImGui::GetCursorScreenPos().x + (avail - rowW) * 0.5f + R,
                    ImGui::GetCursorScreenPos().y + R + 2);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImU32 on  = ImGui::ColorConvertFloat4ToU32(accent);
        ImU32 off = IM_COL32(90, 94, 112, 255);
        for (int i = 0; i < total; ++i) {
            ImVec2 c(base.x + i * STEP, base.y);
            if (i < done)
                dl->AddCircleFilled(c, R, on);
            else
                dl->AddCircle(c, R, off, 20, 1.5f);
        }
        ImGui::Dummy(ImVec2(avail, R * 2 + 6));
    }

    // --- controls ------------------------------------------------------------------
    if (ImGui::Button(timer.paused() ? "Start" : "Pause", ImVec2(-1, 32))) {
        timer.togglePause();
    }
    {
        float w = (ImGui::GetContentRegionAvail().x - 8.0f) * 0.5f;
        if (ImGui::Button("Skip", ImVec2(w, 28)))
            timer.skip();
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(w, 28)))
            timer.reset();
    }

    CenteredText(HintFor(timer.state()));

    // --- footer ----------------------------------------------------------------------
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.52f, 0.55f, 0.62f, 1.0f));
    CenteredText("Ctrl+Alt+P start/pause - drag to move - right-click menu");
    ImGui::PopStyleColor();

    // --- click-drag to move (GIF + timer count as drag zones) --------------------------
    bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    bool blocked = ImGui::IsAnyItemHovered() && !app.gifHovered;
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        app.dragArmed = false;
    else if (!app.dragArmed && hovered && !blocked &&
             ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f)) {
        OsDragWindow(app);
    }

    // --- right-click context menu ---------------------------------------------------------
    if (hovered && !blocked &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("##ctx");
    }
    if (ImGui::BeginPopup("##ctx")) {
        if (ImGui::MenuItem("Collapse to mini"))
            app.setExpanded(false);
        if (ImGui::MenuItem(timer.paused() ? "Start" : "Pause"))
            timer.togglePause();
        if (ImGui::MenuItem("Skip phase"))
            timer.skip();
        if (ImGui::MenuItem("Reset timer"))
            timer.reset();
        if (ImGui::MenuItem("Settings..."))
            app.showSettings = true;
        ImGui::Separator();
        if (ImGui::MenuItem("Quit"))
            app.running = false;
        ImGui::EndPopup();
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    // --- M5 settings panel ---------------------------------------------------------------
    if (app.showSettings) {
        ImGui::SetNextWindowPos(ImVec2(16, 28));
        ImGui::SetNextWindowSize(ImVec2(288, 424));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg,
                              ImVec4(0.10f, 0.105f, 0.14f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 5));
        ImGui::Begin("Settings", &app.showSettings,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        bool changed = false;
        ImGui::TextUnformatted("Work (minutes)");
        changed |= ImGui::SliderInt("##work", &app.settings.workMin, 1, 60);
        ImGui::TextUnformatted("Short break (minutes)");
        changed |= ImGui::SliderInt("##short", &app.settings.shortMin, 1, 30);
        ImGui::TextUnformatted("Long break (minutes)");
        changed |= ImGui::SliderInt("##long", &app.settings.longMin, 1, 45);
        ImGui::TextUnformatted("Work sessions per cycle");
        changed |= ImGui::SliderInt("##cycles", &app.settings.cycles, 2, 8);
        if (changed) {
            app.syncConfigToTimer();
            app.saveSettings();
        }

        ImGui::Spacing();
        if (ImGui::Checkbox("Sound on session end", &app.settings.sound))
            app.saveSettings();
        if (ImGui::Checkbox("Always on top", &app.settings.alwaysOnTop)) {
            app.applyWindowFlags();
            app.saveSettings();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextUnformatted(app.hotkeyActive
                                   ? "Hotkey Ctrl+Alt+P: active"
                                   : "Hotkey Ctrl+Alt+P: unavailable");

        ImGui::Spacing();
        if (ImGui::Button("Reset defaults", ImVec2(-1, 26))) {
            bool keepTop = app.settings.alwaysOnTop;
            app.settings.resetDefaults();
            app.settings.alwaysOnTop = keepTop;  // keep current pin state
            app.syncConfigToTimer();
            app.applyWindowFlags();
            app.saveSettings();
        }
        if (ImGui::Button("Close", ImVec2(-1, 26)))
            app.showSettings = false;

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    }
}

// ---------------------------------------------------------------------------
void RenderUI(App& app) {
    if (app.expanded)
        RenderFull(app);
    else
        RenderMini(app);
}
