#pragma once

#include "model/MidiEvent.h"
#include <vector>

// Turns each ramp into the steps it sends, in tick order. The result has no ramps.
std::vector<MidiEvent> expandControllerRamps(const std::vector<MidiEvent>& events);
