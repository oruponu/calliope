#pragma once

#include "model/event/MetaEvent.h"
#include "model/harmony/ChordChange.h"
#include "model/harmony/KeySignatureChange.h"
#include "model/timeline/TimelineMap.h"
#include "model/track/MidiTrack.h"
#include <vector>

struct SequenceContents
{
    TimelineMap timeline;
    std::vector<MidiTrack> tracks;
    std::vector<KeySignatureChange> keySignatureChanges;
    std::vector<ChordChange> chordChanges;
    std::vector<MetaEvent> metaEvents;
};
