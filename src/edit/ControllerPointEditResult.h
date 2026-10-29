#pragma once

#include "model/event/MidiEvent.h"
#include <set>
#include <vector>

struct ControllerPointEditResult
{
    std::vector<MidiEvent> events;
    // Indices into events.
    std::set<int> selected;

    bool operator==(const ControllerPointEditResult&) const = default;
};
