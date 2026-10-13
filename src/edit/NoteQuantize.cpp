#include "edit/NoteQuantize.h"
#include <limits>

std::vector<NoteEdit> NoteQuantize::startsToGrid(const std::vector<std::pair<NoteRef, MidiNote>>& notes, int gridTicks)
{
    std::vector<NoteEdit> edits;
    if (gridTicks < 1)
        return edits;

    const long long grid = gridTicks;
    for (const auto& [ref, note] : notes)
    {
        long long start = (note.startTick + grid / 2) / grid * grid;
        // A note may end at the last int tick at most, as on loading; the earlier line always keeps it there.
        if (start + note.duration > std::numeric_limits<int>::max())
            start = note.startTick / grid * grid;
        if (start != note.startTick)
            edits.push_back({ref, {.startTick = static_cast<int>(start), .heldLength = note.duration}});
    }
    return edits;
}
