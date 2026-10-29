#include "edit/event/NoteQuantize.h"
#include <limits>

std::vector<NoteEdit> NoteQuantize::startsToGrid(const std::vector<std::pair<NoteRef, MidiNote>>& notes,
                                                 const TimelineGrid& grid)
{
    std::vector<NoteEdit> edits;
    for (const auto& [ref, note] : notes)
    {
        int start = grid.round(note.startTick);
        // A note may end at the last int tick at most, as on loading; the earlier line always keeps it there.
        if (static_cast<long long>(start) + note.duration > std::numeric_limits<int>::max())
            start = grid.floor(note.startTick);
        if (start != note.startTick)
            edits.push_back({ref, {.startTick = start, .heldLength = note.duration}});
    }
    return edits;
}
