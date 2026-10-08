#pragma once

#include "audio/ClickVoice.h"
#include <atomic>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

class MetronomeProcessor : public juce::AudioProcessor
{
public:
    static constexpr int accentNoteNumber = 76;
    static constexpr int plainNoteNumber = 77;

    MetronomeProcessor();

    void setGain(float gain);

    const juce::String getName() const override { return "Metronome"; }

    void prepareToPlay(double sampleRate, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return ClickVoice::lengthSeconds; }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    juce::MidiMessageCollector collector;

private:
    ClickVoice voice;
    std::atomic<float> gain{1.0f};
};
