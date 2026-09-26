#pragma once

#include "model/MidiSequence.h"
#include "model/TrackId.h"
#include <unordered_set>
#include <vector>

struct PluginSyncPlan
{
    std::vector<TrackId> toRetire;
    std::vector<TrackId> toDestroy;
    std::vector<TrackId> toCreate;
};

PluginSyncPlan planPluginSync(const MidiSequence& sequence, const std::vector<TrackId>& liveIds,
                              const std::unordered_set<TrackId>& failedIds);
