#pragma once

#include "model/TrackTimedContents.h"
#include "model/event/MetaEvent.h"
#include "model/harmony/ChordChange.h"
#include "model/harmony/KeySignatureChange.h"
#include "model/timeline/TempoChange.h"
#include "model/timeline/TimeSignatureChange.h"
#include "model/timeline/TimelineMap.h"
#include <vector>

// Every part of the song that carries a tick position.
struct TimedContents
{
    int ticksPerQuarterNote = TimelineMap::defaultTicksPerQuarterNote;
    std::vector<TempoChange> tempoChanges;
    std::vector<TimeSignatureChange> timeSignatureChanges;
    std::vector<KeySignatureChange> keySignatureChanges;
    std::vector<ChordChange> chordChanges;
    std::vector<MetaEvent> metaEvents;
    std::vector<TrackTimedContents> tracks;

    bool operator==(const TimedContents&) const = default;
};
