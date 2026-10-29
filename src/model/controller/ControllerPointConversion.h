#pragma once

#include "model/event/MidiEvent.h"
#include <vector>

// Turns each step change of a controller that can ramp into the points the lane draws: a hold point at the previous
// value, then the new value on the same tick, both ramps. Returns the events in tick order; converting again changes
// nothing.
std::vector<MidiEvent> convertStepsToPoints(const std::vector<MidiEvent>& events);

// The reverse for the hold points only: each one is removed and the switch after it becomes a step again, so values
// merged in before it can be held. Returns the events in tick order.
std::vector<MidiEvent> convertHoldsToSteps(const std::vector<MidiEvent>& events);

// For each event, whether playback and export leave it out: a point overwritten on its own tick by the next event of
// its kind, without changing the value before it. Such points only shape the lane.
std::vector<bool> unsentControllerPoints(const std::vector<MidiEvent>& events);
