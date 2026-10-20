#pragma once

#include "engine/ChaseItem.h"
#include "engine/ChaseTarget.h"
#include "engine/PlaybackSnapshot.h"
#include <vector>

struct ChaseTargetState
{
    ChaseTarget target;
    // The context of the target's first event, used to send the items.
    PlaybackTrackContext ctx;
    std::vector<ChaseItem> items;
};
