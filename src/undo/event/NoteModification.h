#pragma once

#include "model/event/MidiNote.h"

struct NoteModification
{
    int trackIndex;
    int noteIndex;
    MidiNote before;
    MidiNote after;
};
