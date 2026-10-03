#pragma once

#include "model/MidiNote.h"

namespace NoteEdits
{
MidiNote afterStartResize(const MidiNote& note, int deltaTick, int minDuration);
MidiNote afterEndResize(const MidiNote& note, int deltaTick, int minDuration);
} // namespace NoteEdits
