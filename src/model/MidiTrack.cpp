#include "MidiTrack.h"
#include <algorithm>

void MidiTrack::addNote(const MidiNote& note)
{
    notes.push_back(note);
}

void MidiTrack::removeNote(int index)
{
    notes.erase(notes.begin() + index);
}

void MidiTrack::sortByStartTime()
{
    std::sort(notes.begin(), notes.end(),
              [](const MidiNote& a, const MidiNote& b) { return a.startTick < b.startTick; });
}

const std::vector<MidiNote>& MidiTrack::getNotes() const
{
    return notes;
}

MidiNote& MidiTrack::getNote(int index)
{
    return notes[index];
}

const MidiNote& MidiTrack::getNote(int index) const
{
    return notes[index];
}

int MidiTrack::getNumNotes() const
{
    return static_cast<int>(notes.size());
}
