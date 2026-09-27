#pragma once

#include "model/MidiSequence.h"
#include "model/SequenceContents.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <optional>

class MidiFileIO
{
public:
    static bool save(const MidiSequence& sequence, const juce::File& file);
    static std::optional<SequenceContents> load(const juce::File& file);
};
