#pragma once

#include "model/event/MidiEvent.h"
#include "model/event/MidiNote.h"
#include "model/track/TrackId.h"
#include <map>
#include <vector>

struct TakeTrack
{
    std::vector<MidiNote> notes;
    std::vector<MidiEvent> events;
    // The recorded note id of each note, in the same order.
    std::vector<int> noteIds;

    bool empty() const { return notes.empty() && events.empty(); }
};

// What a recording segment has finished so far, by the track it was recorded onto.
using Take = std::map<TrackId, TakeTrack>;
