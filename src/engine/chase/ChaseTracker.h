#pragma once

#include "engine/chase/ChaseItem.h"
#include "engine/chase/ChaseScan.h"
#include "engine/chase/ChaseTarget.h"
#include "engine/playback/PlaybackTrackContext.h"
#include <utility>
#include <vector>

// Follows the state each target has been sent, so that a later change is compared with what the instruments have.
class ChaseTracker
{
public:
    void clear();
    // Ignores tracks without output.
    void record(const PlaybackTrackContext& ctx, const MidiEvent& event);
    // The sent state of target as items in the order they are sent, each event carrying tick.
    std::vector<ChaseItem> itemsOf(const ChaseTarget& target, int tick) const;

private:
    std::vector<std::pair<ChaseTarget, ChaseScan>> scans;
};
