#pragma once

#include "model/MidiNote.h"

// A note being drawn, not yet added to its track.
struct DraftNote
{
    int trackIndex = -1;
    MidiNote note;

    bool operator==(const DraftNote&) const = default;
};
