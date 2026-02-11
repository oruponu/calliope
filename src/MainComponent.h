#pragma once

#include "audio/MidiDeviceOutput.h"
#include "engine/PlaybackEngine.h"
#include "model/MidiSequence.h"
#include "ui/PianoRollComponent.h"
#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void buildTestSequence();

    MidiSequence sequence;
    PlaybackEngine playbackEngine;
    MidiDeviceOutput midiOutput;

    PianoRollComponent pianoRoll;
    juce::Viewport viewport;

    juce::TextButton playButton{"Play"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
