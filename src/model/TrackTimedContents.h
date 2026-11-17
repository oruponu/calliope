#pragma once

#include "model/event/MetaEvent.h"
#include "model/event/MidiEvent.h"
#include "model/event/MidiNote.h"
#include "model/event/SysExEvent.h"
#include "model/track/TrackId.h"
#include <vector>

struct TrackTimedContents
{
    TrackId track{};
    std::vector<MidiNote> notes;
    std::vector<MidiEvent> events;
    std::vector<SysExEvent> sysExEvents;
    std::vector<MetaEvent> metaEvents;

    bool operator==(const TrackTimedContents&) const = default;
};
