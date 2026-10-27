#include "timer.h"

void PomodoroTimer::applyConfig(const PomodoroConfig& cfg) {
    cfg_ = cfg;
    if (cfg_.workMin < 1)   cfg_.workMin = 1;
    if (cfg_.shortMin < 1)  cfg_.shortMin = 1;
    if (cfg_.longMin < 1)   cfg_.longMin = 1;
    if (cfg_.cycles < 2)    cfg_.cycles = 2;
    if (cfg_.cycles > 8)    cfg_.cycles = 8;
    if (remainingMs_ > phaseTotalMs())
        remainingMs_ = phaseTotalMs();
}

uint64_t PomodoroTimer::phaseTotalMs() const {
    switch (state_) {
        case PomState::WORK:        return minutesToMs(cfg_.workMin);
        case PomState::SHORT_BREAK: return minutesToMs(cfg_.shortMin);
        case PomState::LONG_BREAK:  return minutesToMs(cfg_.longMin);
    }
    return minutesToMs(cfg_.workMin);
}

void PomodoroTimer::tick(uint64_t deltaMs) {
    if (paused_ || deltaMs == 0)
        return;
    // Consume the delta, possibly across several phase boundaries.
    while (deltaMs >= remainingMs_) {
        deltaMs -= remainingMs_;
        advance(false);
        if (paused_)
            return;
    }
    remainingMs_ -= deltaMs;
}

void PomodoroTimer::advance(bool silent) {
    if (state_ == PomState::WORK) {
        ++completedWork_;
        state_ = (completedWork_ % cfg_.cycles == 0)
                     ? PomState::LONG_BREAK
                     : PomState::SHORT_BREAK;
    } else {
        state_ = PomState::WORK;
    }
    remainingMs_ = phaseTotalMs();
    if (!silent && onPhaseEnd)
        onPhaseEnd(state_);
}

void PomodoroTimer::skip() {
    advance(false);  // fire callback so UI/GIF updates
}

void PomodoroTimer::reset() {
    state_ = PomState::WORK;
    completedWork_ = 0;
    remainingMs_ = phaseTotalMs();
    paused_ = true;
    if (onPhaseEnd)
        onPhaseEnd(state_);  // fire so GIF resets to rocket
}

double PomodoroTimer::progress() const {
    uint64_t total = phaseTotalMs();
    if (total == 0)
        return 0.0;
    double done = static_cast<double>(total - remainingMs_);
    if (done < 0.0) done = 0.0;
    if (done > static_cast<double>(total)) done = static_cast<double>(total);
    return done / static_cast<double>(total);
}

int PomodoroTimer::cyclePosition() const {
    return completedWork_ % cfg_.cycles;
}

const char* PomodoroTimer::stateLabel() const {
    switch (state_) {
        case PomState::WORK:        return "WORK";
        case PomState::SHORT_BREAK: return "SHORT BREAK";
        case PomState::LONG_BREAK:  return "LONG BREAK";
    }
    return "WORK";
}
