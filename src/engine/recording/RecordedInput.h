#pragma once

#include "engine/transport/ClockPosition.h"
#include "engine/transport/LoopRange.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <string>

struct RecordedInput
{
    std::string sourceId;
    juce::MidiMessage message;
    ClockPosition position;
    // The clock's loop when the input was placed.
    LoopRange loop;
    // Played before the count-in ended. Its position is the count-in end, which may still move.
    bool countIn = false;
};
