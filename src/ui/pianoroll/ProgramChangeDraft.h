#pragma once

#include "model/MidiEvent.h"
#include <vector>

struct ProgramChangeDraft
{
    int trackIndex = -1;
    int tick = 0;
    // The track's events when the browser opened; a commit onto any other list is dropped.
    std::vector<MidiEvent> eventsAtOpen;
};
