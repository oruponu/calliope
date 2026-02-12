#pragma once

#include "MidiEvent.h"
#include "MidiNote.h"
#include <string>
#include <vector>

class MidiTrack
{
public:
    void addNote(const MidiNote& note);
    void removeNote(int index);
    void clear();
    void sortByStartTime();

    const std::vector<MidiNote>& getNotes() const;
    MidiNote& getNote(int index);
    const MidiNote& getNote(int index) const;
    int getNumNotes() const;

    void addEvent(const MidiEvent& event);
    void removeEvent(int index);
    const std::vector<MidiEvent>& getEvents() const;
    const MidiEvent& getEvent(int index) const;
    int getNumEvents() const;

    bool isMuted() const;
    void setMuted(bool muted);

    bool isSolo() const;
    void setSolo(bool solo);

    const std::string& getName() const;
    void setName(const std::string& name);

private:
    std::vector<MidiNote> notes;
    std::vector<MidiEvent> events;
    std::string name;
    bool muted = false;
    bool solo = false;
};
