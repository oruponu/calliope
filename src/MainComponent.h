#pragma once

#include "audio/MidiDeviceOutput.h"
#include "engine/PlaybackEngine.h"
#include "model/MidiSequence.h"
#include "ui/PianoRollComponent.h"
#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent : public juce::Component, private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildTestSequence();
    void updatePositionLabel();

    MidiSequence sequence;
    PlaybackEngine playbackEngine;
    MidiDeviceOutput midiOutput;

    PianoRollComponent pianoRoll;
    juce::Viewport viewport;

    juce::TextButton playButton{"Play"};
    juce::TextButton stopButton{"Stop"};
    juce::Label bpmLabel{"", "BPM"};
    juce::Slider bpmSlider;
    juce::Label positionLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
