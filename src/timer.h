#pragma once

#include <cstdint>
#include <functional>

// M2 — Timer core -----------------------------------------------------------
enum class PomState { WORK, SHORT_BREAK, LONG_BREAK };

struct PomodoroConfig {
    int workMin  = 25;
    int shortMin = 5;
    int longMin  = 15;
    int cycles   = 4;  // work sessions before a long break
};

class PomodoroTimer {
public:
    using PhaseCallback = std::function<void(PomState newState)>;

    PomodoroTimer() = default;

    void applyConfig(const PomodoroConfig& cfg);
    void tick(uint64_t deltaMs);  // call every frame; no threads needed
    void togglePause() { paused_ = !paused_; }
    void setPaused(bool p) { paused_ = p; }
    bool paused() const { return paused_; }
    void skip();  // jump to next phase silently
    void reset(); // back to a fresh WORK phase, paused

    PomState   state() const { return state_; }
    uint64_t   remainingMs() const { return remainingMs_; }
    uint64_t   phaseTotalMs() const;
    double     progress() const;  // 0..1 elapsed fraction of current phase
    int        completedWork() const { return completedWork_; }
    int        cyclePosition() const;  // finished sessions in current cycle
    const char* stateLabel() const;

    PhaseCallback onPhaseEnd;  // fired on automatic phase completion

private:
    void advance(bool silent);
    static uint64_t minutesToMs(int m) {
        return static_cast<uint64_t>(m < 1 ? 1 : m) * 60000ull;
    }

    PomodoroConfig cfg_;
    PomState  state_       = PomState::WORK;
    uint64_t  remainingMs_ = 25 * 60000ull;
    int       completedWork_ = 0;
    bool      paused_      = true;
};
