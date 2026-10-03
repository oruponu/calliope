#pragma once

#include "model/MidiNote.h"
#include <vector>

struct NoteShift
{
    int deltaTick;
    int deltaNote;
};

namespace NoteEdits
{
MidiNote afterStartResize(const MidiNote& note, int deltaTick, int minDuration);
MidiNote afterEndResize(const MidiNote& note, int deltaTick, int minDuration);

bool canShiftTime(const std::vector<MidiNote>& notes, int deltaTick);
bool canShiftPitch(const std::vector<MidiNote>& notes, int deltaNote);
NoteShift clampShift(const std::vector<MidiNote>& notes, NoteShift shift);
} // namespace NoteEdits
