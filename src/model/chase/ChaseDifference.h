#pragma once

#include "model/chase/ChaseItem.h"
#include "model/event/MidiEvent.h"
#include <vector>

// The events that take an instrument holding sent to target. Items of target that differ from sent; every item when
// resendAll or when target's program differs from sent's; the selection again after any parameter; the bank of the
// current program when a pending bank is no longer pending. Items only sent has are not undone.
std::vector<MidiEvent> chaseEventsToReach(const std::vector<ChaseItem>& sent, const std::vector<ChaseItem>& target,
                                          bool resendAll);
