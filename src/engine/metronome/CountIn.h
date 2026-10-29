#pragma once

#include "engine/metronome/MetronomeClicks.h"
#include "model/timeline/TimeSignatureChange.h"
#include <vector>

// Whole bars of clicks that end where playback starts, on the beats of the time signature in force there. Ticks count
// from the start of the count-in.
struct CountIn
{
    int lengthTicks = 0;
    int ticksPerBeat = 0;
    int beatsPerBar = 0;
    // The first beat at or after the start of the count-in, and its index within its bar.
    int firstBeatTick = 0;
    int firstBeatInBar = 0;

    bool operator==(const CountIn&) const = default;
};

// The beats run on back from the time signature section that startTick lies in, past its start or tick 0 if need be.
CountIn makeCountIn(const std::vector<TimeSignatureChange>& timeSignatures, int ticksPerQuarterNote, int startTick,
                    int bars);

// On the same bars as the count-in clicks, so a count-in to it starts with the accent.
int barStartAtOrBefore(const std::vector<TimeSignatureChange>& timeSignatures, int ticksPerQuarterNote, int tick);

// Clicks in [fromTick, toTick), never past the end of the count-in.
std::vector<MetronomeClick> countInClicksInRange(const CountIn& countIn, int fromTick, int toTick);
