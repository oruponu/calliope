#pragma once

#include "edit/NoteEdit.h"
#include "edit/NoteRef.h"
#include "model/MidiNote.h"
#include "model/TimelineGrid.h"
#include <utility>
#include <vector>

namespace NoteQuantize
{
// Moves each start to the nearest line of the grid, a tie going to the later line, and keeps the length. A note whose
// end would pass the last int tick goes to the earlier line instead. The length also goes in as the held length, so a
// held note that lands at or after the playhead ends with it. Notes already on the grid get no edit.
std::vector<NoteEdit> startsToGrid(const std::vector<std::pair<NoteRef, MidiNote>>& notes, const TimelineGrid& grid);
} // namespace NoteQuantize
