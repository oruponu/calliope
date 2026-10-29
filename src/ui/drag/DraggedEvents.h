#pragma once

#include "model/event/MidiEvent.h"
#include <set>
#include <vector>

// A track's events as a controller point drag shows them.
struct DraggedEvents
{
    int trackIndex = -1;
    std::vector<MidiEvent> events;
    // Indices into events of the dragged points.
    std::set<int> dragged;
    // Index into events of the grabbed point, whose row is kept on screen; -1 for none.
    int grabbed = -1;

    bool operator==(const DraggedEvents&) const = default;
};
