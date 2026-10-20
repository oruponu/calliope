#include "engine/Chase.h"
#include "engine/ChaseScan.h"
#include <algorithm>
#include <utility>

namespace
{
struct ScannedTarget
{
    ChaseTarget target;
    PlaybackTrackContext firstCtx;
    ChaseScan scan;
};
} // namespace

std::vector<ChaseTargetState> chaseStates(const PlaybackSnapshot& snap, int tick)
{
    std::vector<ScannedTarget> scanned;
    for (const auto& se : snap.events)
    {
        if (se.event.tick >= tick)
            break;
        if (se.ctx.destination == MidiTrack::OutputDestination::None)
            continue;
        const ChaseTarget target = chaseTargetOf(se.ctx);
        auto it = std::find_if(scanned.begin(), scanned.end(), [&](const auto& s) { return s.target == target; });
        if (it == scanned.end())
            it = scanned.insert(scanned.end(), {target, se.ctx, ChaseScan{}});
        it->scan.apply(se.event);
    }

    std::vector<ChaseTargetState> states;
    for (const auto& s : scanned)
    {
        auto items = s.scan.items(tick);
        if (!items.empty())
            states.push_back({s.target, s.firstCtx, std::move(items)});
    }
    return states;
}

std::vector<ScheduledEvent> chaseEvents(const PlaybackSnapshot& snap, int tick)
{
    std::vector<ScheduledEvent> events;
    for (const auto& state : chaseStates(snap, tick))
        for (const auto& item : state.items)
            for (const auto& event : item.events)
                events.push_back({state.ctx, event});
    return events;
}
