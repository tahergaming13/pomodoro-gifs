// M2 logic test: phase transitions, skip, reset, progress. Pure C++, no SDL.
#include "timer.h"
#include <cassert>
#include <cstdio>

int main() {
    PomodoroTimer t;
    PomodoroConfig c;
    c.workMin = 1; c.shortMin = 1; c.longMin = 1; c.cycles = 2;
    t.applyConfig(c);

    int callbacks = 0;
    PomState last = PomState::WORK;
    t.onPhaseEnd = [&](PomState s) { ++callbacks; last = s; };

    assert(t.paused() && t.state() == PomState::WORK);
    assert(t.remainingMs() == 60000);

    t.togglePause();
    t.tick(30000);
    assert(t.remainingMs() == 30000);
    assert(t.progress() > 0.49 && t.progress() < 0.51);

    t.tick(30000);  // -> SHORT_BREAK (1st work done, cycles=2)
    assert(callbacks == 1 && last == PomState::SHORT_BREAK);
    assert(t.state() == PomState::SHORT_BREAK && t.cyclePosition() == 1);

    t.tick(60000);  // -> WORK
    assert(t.state() == PomState::WORK && callbacks == 2);

    t.tick(60000);  // 2nd work done -> LONG_BREAK
    assert(t.state() == PomState::LONG_BREAK && callbacks == 3);

    t.skip();  // silent -> WORK, no callback
    assert(t.state() == PomState::WORK && callbacks == 3);

    t.reset();
    assert(t.state() == PomState::WORK && t.paused());
    assert(t.completedWork() == 0 && t.remainingMs() == 60000);

    // Multi-phase jump in one huge tick.
    t.togglePause();
    t.tick(60000 * 5 + 1000);
    assert(callbacks > 3);
    std::printf("timer_test: ALL OK (callbacks=%d)\n", callbacks);
    return 0;
}
