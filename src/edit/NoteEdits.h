#pragma once

#include "model/MidiNote.h"
#include "model/TimelineGrid.h"
#include <optional>
#include <vector>

struct NoteShift
{
    int deltaTick;
    int deltaNote;
};

namespace NoteEdits
{
// The start stays at or after 0 and at or before the last line before the end.
MidiNote afterStartResize(const MidiNote& note, int deltaTick, const TimelineGrid& grid);
// The end stays at or after the first line after the start.
MidiNote afterEndResize(const MidiNote& note, int deltaTick, const TimelineGrid& grid);
// From the tick to the next line (forward) or the previous one; nullopt when that line is outside int.
std::optional<int> deltaToAdjacentLine(int tick, bool forward, const TimelineGrid& grid);

bool canShiftTime(const std::vector<MidiNote>& notes, int deltaTick);
bool canShiftPitch(const std::vector<MidiNote>& notes, int deltaNote);
NoteShift clampShift(const std::vector<MidiNote>& notes, NoteShift shift);
} // namespace NoteEdits
