#pragma once

#include "edit/time/LineCrossing.h"
#include "model/event/MidiEvent.h"
#include <vector>

// For each controller kind that can ramp: the line from the last point before tick (at or before it when fromTick)
// to the first point after that one, when that point is a ramp to another value and lies at or after tick.
// In ControllerKind order (type, then number).
std::vector<LineCrossing> lineCrossings(const std::vector<MidiEvent>& events, int tick, bool fromTick);
