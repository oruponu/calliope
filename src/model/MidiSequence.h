#pragma once

#include "MidiTrack.h"
#include <vector>

struct TempoChange
{
    int tick;
    double bpm;
};

struct TimeSignatureChange
{
    int tick;
    int numerator;
    int denominator;
};

struct BarBeatTick
{
    int bar;  // 1-based
    int beat; // 1-based
    int tick; // tick within beat
};

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

    double getTempoAt(int tick) const;
    TimeSignatureChange getTimeSignatureAt(int tick) const;

    const std::vector<TempoChange>& getTempoChanges() const;
    const std::vector<TimeSignatureChange>& getTimeSignatureChanges() const;

    void addTempoChange(int tick, double bpm);
    void addTimeSignatureChange(int tick, int num, int den);

    double ticksToSeconds(int ticks) const;
    int secondsToTicks(double seconds) const;

    BarBeatTick tickToBarBeatTick(int tick) const;

private:
    std::vector<MidiTrack> tracks;
    std::vector<TempoChange> tempoChanges;
    std::vector<TimeSignatureChange> timeSignatureChanges;
    int ticksPerQuarterNote = defaultTicksPerQuarterNote;
};
