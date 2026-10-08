#include "audio/MetronomeProcessor.h"
#include <algorithm>

MetronomeProcessor::MetronomeProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Clicks may be queued before the graph is prepared (e.g. no audio device could be opened).
    collector.reset(44100.0);
}

void MetronomeProcessor::setGain(float newGain)
{
    gain.store(newGain);
}

void MetronomeProcessor::prepareToPlay(double sampleRate, int)
{
    collector.reset(sampleRate);
    voice.prepare(sampleRate);
}

void MetronomeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    buffer.clear();
    midi.clear();
    collector.removeNextBlockOfMessages(midi, buffer.getNumSamples());

    const float currentGain = gain.load();
    auto* const* channels = buffer.getArrayOfWritePointers();
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    int rendered = 0;
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (!message.isNoteOn())
            continue;
        const int position = std::clamp(metadata.samplePosition, rendered, numSamples);
        voice.render(channels, numChannels, rendered, position - rendered, currentGain);
        voice.trigger(message.getNoteNumber() == accentNoteNumber);
        rendered = position;
    }
    voice.render(channels, numChannels, rendered, numSamples - rendered, currentGain);
    midi.clear();
}
