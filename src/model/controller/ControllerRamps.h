#pragma once

#include "model/event/MidiEvent.h"
#include <vector>

// Turns each ramp into the steps it sends, in tick order. The result has no ramps.
std::vector<MidiEvent> expandControllerRamps(const std::vector<MidiEvent>& events);

// The value a ramp from (fromTick, fromValue) to (toTick, toValue) sends at tick, for fromTick <= tick < toTick.
int rampValueAt(int fromTick, int fromValue, int toTick, int toValue, int tick);
