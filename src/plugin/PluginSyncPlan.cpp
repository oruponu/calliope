#include "plugin/PluginSyncPlan.h"
#include <algorithm>

PluginSyncPlan planPluginSync(const MidiSequence& sequence, const std::vector<TrackId>& liveIds,
                              const std::unordered_set<TrackId>& failedIds)
{
    PluginSyncPlan plan;

    for (TrackId id : liveIds)
    {
        const int index = sequence.indexOf(id);
        if (index < 0)
            plan.toRetire.push_back(id);
        else if (sequence.getTrack(index).getPluginAssignment() == nullptr)
            plan.toDestroy.push_back(id);
    }

    for (int i = 0; i < sequence.getNumTracks(); ++i)
    {
        const auto& track = sequence.getTrack(i);
        const TrackId id = track.getId();
        if (track.getPluginAssignment() != nullptr && std::ranges::find(liveIds, id) == liveIds.end() &&
            !failedIds.contains(id))
            plan.toCreate.push_back(id);
    }

    return plan;
}
