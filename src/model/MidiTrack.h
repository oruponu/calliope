#pragma once

#include "MidiNote.h"
#include <vector>

class MidiTrack
{
public:
    void addNote(const MidiNote& note);
    void removeNote(int index);
    void sortByStartTime();

    const std::vector<MidiNote>& getNotes() const;
    MidiNote& getNote(int index);
    const MidiNote& getNote(int index) const;
    int getNumNotes() const;

private:
    std::vector<MidiNote> notes;
};
