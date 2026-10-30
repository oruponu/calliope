#pragma once

#include "engine/chase/ChaseItem.h"
#include "engine/chase/ChaseTarget.h"
#include "engine/playback/PlaybackTrackContext.h"
#include <vector>

struct ChaseTargetState
{
    ChaseTarget target;
    // The context of the target's first event, used to send the items.
    PlaybackTrackContext ctx;
    std::vector<ChaseItem> items;
};
