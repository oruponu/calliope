#pragma once

#include "MidiTrack.h"
#include <string>
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

struct KeySignatureChange
{
    int tick;
    int sharpsOrFlats; // -7..+7 (negative=flats, positive=sharps)
    bool isMinor;
};

struct ChordChange
{
    int tick;
    int chordRoot; // XF format: upper nibble=accidental(0-6), lower nibble=note(0-7)
    int chordType; // XF format: 0-34
    int bassRoot;  // same as chordRoot, 0x7F=none
    int bassType;  // same as chordType, 0x7F=none
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
    bool isAnySolo() const;

    void setBpm(double newBpm);
    double getBpm() const;
    int getTicksPerQuarterNote() const;
    void setTicksPerQuarterNote(int ppq);

    double getTempoAt(int tick) const;
    TimeSignatureChange getTimeSignatureAt(int tick) const;

    KeySignatureChange getKeySignatureAt(int tick) const;

    const std::vector<TempoChange>& getTempoChanges() const;
    const std::vector<TimeSignatureChange>& getTimeSignatureChanges() const;
    const std::vector<KeySignatureChange>& getKeySignatureChanges() const;
    const std::vector<ChordChange>& getChordChanges() const;

    void addTempoChange(int tick, double bpm);
    void addTimeSignatureChange(int tick, int num, int den);
    void addKeySignatureChange(int tick, int sharpsOrFlats, bool isMinor);
    void addChordChange(int tick, int chordRoot, int chordType, int bassRoot, int bassType);

    static std::string keySignatureToString(int sharpsOrFlats, bool isMinor);
    static std::string chordToString(const ChordChange& chord);

    double ticksToSeconds(int ticks) const;
    int secondsToTicks(double seconds) const;

    BarBeatTick tickToBarBeatTick(int tick) const;
    int barStartToTick(int barNumber) const;

private:
    std::vector<MidiTrack> tracks;
    std::vector<TempoChange> tempoChanges;
    std::vector<TimeSignatureChange> timeSignatureChanges;
    std::vector<KeySignatureChange> keySignatureChanges;
    std::vector<ChordChange> chordChanges;
    int ticksPerQuarterNote = defaultTicksPerQuarterNote;
};
