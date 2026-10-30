#pragma once

// A pass is the stretch of playback since playback started, the loop wrapped or the playhead jumped.
struct ClockPosition
{
    int pass = 0;
    int tick = 0;

    bool operator==(const ClockPosition&) const = default;
};
