#include "edit/NoteEdits.h"
#include <algorithm>
#include <limits>

MidiNote NoteEdits::afterStartResize(const MidiNote& note, int deltaTick, const TimelineGrid& grid)
{
    const int endTick = note.endTick();
    const int latestStart = grid.previous(endTick).value_or(std::numeric_limits<int>::min());
    MidiNote resized = note;
    resized.startTick = std::clamp(note.startTick + deltaTick, 0, std::max(0, latestStart));
    resized.duration = endTick - resized.startTick;
    return resized;
}

MidiNote NoteEdits::afterEndResize(const MidiNote& note, int deltaTick, const TimelineGrid& grid)
{
    const int earliestEnd = grid.next(note.startTick).value_or(std::numeric_limits<int>::max());
    MidiNote resized = note;
    resized.duration = std::max(earliestEnd, note.endTick() + deltaTick) - note.startTick;
    return resized;
}

std::optional<int> NoteEdits::deltaToAdjacentLine(int tick, bool forward, const TimelineGrid& grid)
{
    const auto line = forward ? grid.next(tick) : grid.previous(tick);
    if (!line)
        return std::nullopt;
    return *line - tick;
}

bool NoteEdits::canShiftTime(const std::vector<MidiNote>& notes, int deltaTick)
{
    return std::ranges::all_of(notes, [deltaTick](const MidiNote& n) { return n.startTick + deltaTick >= 0; });
}

bool NoteEdits::canShiftPitch(const std::vector<MidiNote>& notes, int deltaNote)
{
    return std::ranges::all_of(notes,
                               [deltaNote](const MidiNote& n)
                               {
                                   const int noteNumber = n.noteNumber + deltaNote;
                                   return noteNumber >= 0 && noteNumber <= 127;
                               });
}

NoteShift NoteEdits::clampShift(const std::vector<MidiNote>& notes, NoteShift shift)
{
    int minStart = std::numeric_limits<int>::max();
    int minNote = 127;
    int maxNote = 0;
    for (const auto& n : notes)
    {
        minStart = std::min(minStart, n.startTick);
        minNote = std::min(minNote, n.noteNumber);
        maxNote = std::max(maxNote, n.noteNumber);
    }
    shift.deltaTick = std::max(shift.deltaTick, -minStart);
    shift.deltaNote = std::clamp(shift.deltaNote, -minNote, 127 - maxNote);
    return shift;
}
