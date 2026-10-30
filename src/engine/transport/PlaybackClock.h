#pragma once

#include "engine/transport/ClockPosition.h"
#include "engine/transport/LoopRange.h"

// The playhead as of the latest playback timer callback.
struct PlaybackClock
{
    double timeMs = 0.0;
    double tick = 0.0;
    double ticksPerMs = 0.0;
    LoopRange loop;
    int pass = 0;
    int passStartTick = 0;
    bool passStartedByLoop = false;
};

// Extrapolates the playhead to timeMs (on the Time::getMillisecondCounterHiRes() clock). Crossing the loop end
// moves into the next pass (once at most, like PlaybackEngine); going back before a loop wrap moves into the previous
// one; going back before a jump or the start of playback stops at the pass start.
ClockPosition locate(const PlaybackClock& clock, double timeMs);
