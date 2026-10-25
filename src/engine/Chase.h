#pragma once

#include "engine/ChaseTargetState.h"
#include "engine/ChaseTracker.h"
#include "engine/PlaybackSnapshot.h"
#include <vector>

// The state that playing snap.events from the start up to, but not including, tick leaves each target in, in
// order of each target's first event. Targets without output and targets left with nothing to send are omitted.
// Every event carries tick.
std::vector<ChaseTargetState> chaseStates(const PlaybackSnapshot& snap, int tick);

std::vector<ScheduledEvent> chaseEvents(const PlaybackSnapshot& snap, int tick);

// The targets of the contexts in only one of before and after: those only in after, in its order, then those only
// in before. Targets without output are omitted.
std::vector<ChaseTarget> chaseTargetsChangedBetween(const std::vector<PlaybackTrackContext>& before,
                                                    const std::vector<PlaybackTrackContext>& after);

// What to send at tick after the snapshot changes: targets whose audible contexts changed send the items of their
// state in after that differ from what they have been sent. Targets whose contexts did not change send their entire
// state only when its program is not satisfied by the program sent to the target. A changed program brings every
// item, since some instruments reset controllers on a program change. Items that after does not have are not sent.
std::vector<ScheduledEvent> chaseChanges(const ChaseTracker& sent, const std::vector<PlaybackTrackContext>& before,
                                         const PlaybackSnapshot& after, int tick);
