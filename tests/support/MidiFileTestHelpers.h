#pragma once

#include "io/MidiFileIO.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <optional>

namespace midifiletest
{
inline std::optional<SequenceContents> loadBytes(const juce::MemoryBlock& bytes)
{
    juce::TemporaryFile temp(".mid");
    if (!temp.getFile().replaceWithData(bytes.getData(), bytes.getSize()))
        return std::nullopt;
    return MidiFileIO::load(temp.getFile());
}

inline juce::MemoryBlock toBytes(const juce::MidiFile& midiFile, int format)
{
    juce::MemoryOutputStream out;
    midiFile.writeTo(out, format);
    return out.getMemoryBlock();
}

inline std::optional<SequenceContents> saveAndLoad(const MidiSequence& sequence)
{
    juce::TemporaryFile temp(".mid");
    if (!MidiFileIO::save(sequence, temp.getFile()))
        return std::nullopt;
    return MidiFileIO::load(temp.getFile());
}

inline juce::MidiMessage at(juce::MidiMessage message, double tick)
{
    message.setTimeStamp(tick);
    return message;
}
} // namespace midifiletest
