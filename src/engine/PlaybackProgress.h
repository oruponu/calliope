#pragma once

#include "engine/PlaybackClock.h"
#include <optional>
#include <vector>

// How far playback has sent events, handed over by RecordingInbox together with its queue.
struct PlaybackProgress
{
    // Where the latest playback timer callback finished sending events, in the clock's pass. Playback restarts its
    // cursors at this tick or later when its snapshot changes, so a note added to the take that starts before it is not
    // sent again in this pass.
    ClockPosition sent;
    // Where playback stopped sending each pass it left, by a loop wrap or a jump, since the previous hand-over.
    std::vector<ClockPosition> passEnds;

    // How far playback has sent `pass`: where it left the pass, or where it is now. nullopt when the clock has not
    // reached the pass, skipped it (a jump cancelled a wrap that input announced), or left it before the previous
    // hand-over.
    std::optional<int> sentTickIn(int pass) const
    {
        for (const auto& end : passEnds)
            if (end.pass == pass)
                return end.tick;
        if (sent.pass == pass)
            return sent.tick;
        return std::nullopt;
    }
};
