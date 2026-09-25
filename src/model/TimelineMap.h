#pragma once

#include "model/BarBeatTick.h"
#include "model/TempoChange.h"
#include "model/TimeSignatureChange.h"
#include <vector>

class TimelineMap
{
public:
    static constexpr int defaultTicksPerQuarterNote = 480;
    static constexpr double minBpm = 10.0;
    static constexpr double maxBpm = 400.0;

    TimelineMap();

    int getTicksPerQuarterNote() const;
    void setTicksPerQuarterNote(int ppq);

    const std::vector<TempoChange>& getTempoChanges() const;
    void setTempoChanges(std::vector<TempoChange> changes);
    const std::vector<TimeSignatureChange>& getTimeSignatureChanges() const;
    void setTimeSignatureChanges(std::vector<TimeSignatureChange> changes);

    double getTempoAt(int tick) const;
    TempoChange getTempoChangeAt(int tick) const;
    TimeSignatureChange getTimeSignatureAt(int tick) const;

    double ticksToSeconds(int ticks) const;
    int secondsToTicks(double seconds) const;

    BarBeatTick tickToBarBeatTick(int tick) const;
    int barStartToTick(int barNumber) const;
    int barBeatTickToTick(int bar, int beat, int tickInBeat) const;

private:
    std::vector<TempoChange> tempoChanges;
    std::vector<TimeSignatureChange> timeSignatureChanges;
    int ticksPerQuarterNote = defaultTicksPerQuarterNote;
};
