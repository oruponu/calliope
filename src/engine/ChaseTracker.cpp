#include "engine/ChaseTracker.h"
#include <algorithm>

void ChaseTracker::clear()
{
    scans.clear();
}

void ChaseTracker::record(const PlaybackTrackContext& ctx, const MidiEvent& event)
{
    if (ctx.destination == MidiTrack::OutputDestination::None)
        return;
    const ChaseTarget target = chaseTargetOf(ctx);
    auto it = std::find_if(scans.begin(), scans.end(), [&](const auto& s) { return s.first == target; });
    if (it == scans.end())
        it = scans.insert(scans.end(), {target, ChaseScan{}});
    it->second.apply(event);
}

std::vector<ChaseItem> ChaseTracker::itemsOf(const ChaseTarget& target, int tick) const
{
    const auto it = std::find_if(scans.begin(), scans.end(), [&](const auto& s) { return s.first == target; });
    return it != scans.end() ? it->second.items(tick) : std::vector<ChaseItem>{};
}
