#pragma once

#include "model/TimeSignatureChange.h"
#include <vector>

struct MetronomeClick
{
    int tick;
    bool accent;
    bool operator==(const MetronomeClick&) const = default;
};

// Beat heads in [fromTick, toTick).
std::vector<MetronomeClick> metronomeClicksInRange(const std::vector<TimeSignatureChange>& timeSignatures,
                                                   int ticksPerQuarterNote, int fromTick, int toTick);
