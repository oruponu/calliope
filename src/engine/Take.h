#pragma once

#include "model/MidiEvent.h"
#include "model/MidiNote.h"
#include "model/TrackId.h"
#include <map>
#include <vector>

struct TakeTrack
{
    std::vector<MidiNote> notes;
    std::vector<MidiEvent> events;

    bool empty() const { return notes.empty() && events.empty(); }
};

// What a recording segment has finished so far, by the track it was recorded onto.
using Take = std::map<TrackId, TakeTrack>;

struct HeldTakeNote
{
    TrackId track{};
    int noteNumber = 0;
    int velocity = 0;
    int startTick = 0;

    bool operator==(const HeldTakeNote&) const = default;
};
