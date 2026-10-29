#include "engine/transport/PlaybackClock.h"
#include <algorithm>
#include <cmath>

ClockPosition locate(const PlaybackClock& clock, double timeMs)
{
    double tick = clock.tick + (timeMs - clock.timeMs) * clock.ticksPerMs;
    int pass = clock.pass;
    const int loopLength = clock.loop.end - clock.loop.start;

    if (clock.loop.enabled && loopLength > 0 && clock.tick < clock.loop.end && tick >= clock.loop.end)
    {
        tick -= loopLength;
        if (tick >= clock.loop.end)
            tick = clock.loop.start;
        ++pass;
    }
    else if (tick < clock.passStartTick)
    {
        if (clock.passStartedByLoop && loopLength > 0)
        {
            tick += loopLength;
            --pass;
        }
        else
        {
            tick = clock.passStartTick;
        }
    }
    return {pass, std::max(0, static_cast<int>(std::floor(tick)))};
}
