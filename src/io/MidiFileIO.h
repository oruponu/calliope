#pragma once

#include "../model/MidiSequence.h"
#include <juce_audio_basics/juce_audio_basics.h>

class MidiFileIO
{
public:
    static bool save(const MidiSequence& sequence, const juce::File& file);
    static bool load(MidiSequence& sequence, const juce::File& file);
};
