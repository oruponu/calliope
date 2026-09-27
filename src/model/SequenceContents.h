#pragma once

#include "model/ChordChange.h"
#include "model/KeySignatureChange.h"
#include "model/MidiTrack.h"
#include "model/TimelineMap.h"
#include <vector>

struct SequenceContents
{
    TimelineMap timeline;
    std::vector<MidiTrack> tracks;
    std::vector<KeySignatureChange> keySignatureChanges;
    std::vector<ChordChange> chordChanges;
};
