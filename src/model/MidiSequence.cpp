#include "MidiSequence.h"

MidiSequence::MidiSequence() = default;

void MidiSequence::clear()
{
    tracks.clear();
    bpm = 120.0;
    ticksPerQuarterNote = defaultTicksPerQuarterNote;
}

MidiTrack& MidiSequence::addTrack()
{
    tracks.emplace_back();
    return tracks.back();
}

void MidiSequence::removeTrack(int index)
{
    tracks.erase(tracks.begin() + index);
}

MidiTrack& MidiSequence::getTrack(int index)
{
    return tracks[index];
}

const MidiTrack& MidiSequence::getTrack(int index) const
{
    return tracks[index];
}

int MidiSequence::getNumTracks() const
{
    return static_cast<int>(tracks.size());
}

void MidiSequence::setBpm(double newBpm)
{
    bpm = newBpm;
}

double MidiSequence::getBpm() const
{
    return bpm;
}

int MidiSequence::getTicksPerQuarterNote() const
{
    return ticksPerQuarterNote;
}

void MidiSequence::setTicksPerQuarterNote(int ppq)
{
    ticksPerQuarterNote = ppq;
}

double MidiSequence::ticksToSeconds(int ticks) const
{
    double beatsPerSecond = bpm / 60.0;
    double ticksPerSecond = beatsPerSecond * ticksPerQuarterNote;
    return ticks / ticksPerSecond;
}

int MidiSequence::secondsToTicks(double seconds) const
{
    double beatsPerSecond = bpm / 60.0;
    double ticksPerSecond = beatsPerSecond * ticksPerQuarterNote;
    return static_cast<int>(seconds * ticksPerSecond);
}
