#pragma once

#include "model/controller/ControllerKind.h"
#include "model/event/MidiEvent.h"
#include <optional>
#include <vector>

// The value the lane's line shows at tick: the last point at or before it, ramping towards the next point when that
// one ramps. None before the first point.
std::optional<int> controllerLineValueAt(const std::vector<MidiEvent>& events, const ControllerKind& kind, int tick);
