#pragma once

#include "engine/ChaseTargetState.h"
#include "engine/PlaybackSnapshot.h"
#include <vector>

// The state that playing snap.events from the start up to, but not including, tick leaves each target in, in
// order of each target's first event. Targets without output and targets left with nothing to send are omitted.
// Every event carries tick.
std::vector<ChaseTargetState> chaseStates(const PlaybackSnapshot& snap, int tick);

std::vector<ScheduledEvent> chaseEvents(const PlaybackSnapshot& snap, int tick);
