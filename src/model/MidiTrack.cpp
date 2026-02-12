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

void MidiTrack::clear()
{
    notes.clear();
    events.clear();
}

void MidiTrack::sortByStartTime()
{
    std::sort(notes.begin(), notes.end(),
              [](const MidiNote& a, const MidiNote& b) { return a.startTick < b.startTick; });
    std::sort(events.begin(), events.end(), [](const MidiEvent& a, const MidiEvent& b) { return a.tick < b.tick; });
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

void MidiTrack::addEvent(const MidiEvent& event)
{
    events.push_back(event);
}

void MidiTrack::removeEvent(int index)
{
    events.erase(events.begin() + index);
}

const std::vector<MidiEvent>& MidiTrack::getEvents() const
{
    return events;
}

const MidiEvent& MidiTrack::getEvent(int index) const
{
    return events[index];
}

int MidiTrack::getNumEvents() const
{
    return static_cast<int>(events.size());
}

bool MidiTrack::isMuted() const
{
    return muted;
}
void MidiTrack::setMuted(bool m)
{
    muted = m;
}

bool MidiTrack::isSolo() const
{
    return solo;
}
void MidiTrack::setSolo(bool s)
{
    solo = s;
}
