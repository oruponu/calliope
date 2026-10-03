#include "edit/NoteEdits.h"
#include <algorithm>
#include <limits>

MidiNote NoteEdits::afterStartResize(const MidiNote& note, int deltaTick, int minDuration)
{
    const int endTick = note.endTick();
    MidiNote resized = note;
    resized.startTick = std::clamp(note.startTick + deltaTick, 0, std::max(0, endTick - minDuration));
    resized.duration = endTick - resized.startTick;
    return resized;
}

MidiNote NoteEdits::afterEndResize(const MidiNote& note, int deltaTick, int minDuration)
{
    MidiNote resized = note;
    resized.duration = std::max(minDuration, note.duration + deltaTick);
    return resized;
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
