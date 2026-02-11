#pragma once

#include "MidiTrack.h"
#include <vector>

class MidiSequence
{
public:
    static constexpr int defaultTicksPerQuarterNote = 480;

    MidiSequence();

    void clear();

    MidiTrack& addTrack();
    void removeTrack(int index);

    MidiTrack& getTrack(int index);
    const MidiTrack& getTrack(int index) const;
    int getNumTracks() const;

    void setBpm(double newBpm);
    double getBpm() const;
    int getTicksPerQuarterNote() const;
    void setTicksPerQuarterNote(int ppq);

    double ticksToSeconds(int ticks) const;
    int secondsToTicks(double seconds) const;

private:
    std::vector<MidiTrack> tracks;
    double bpm = 120.0;
    int ticksPerQuarterNote = defaultTicksPerQuarterNote;
};
