#pragma once

#include "audio/MidiDeviceOutput.h"
#include "engine/PlaybackEngine.h"
#include "model/MidiSequence.h"
#include "ui/PianoRollComponent.h"
#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent : public juce::Component, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;
    void buildTestSequence();
    void updatePositionLabel();
    void saveFile();
    void loadFile();
    void onSequenceLoaded();

    MidiSequence sequence;
    PlaybackEngine playbackEngine;
    MidiDeviceOutput midiOutput;

    PianoRollComponent pianoRoll;
    juce::Viewport viewport;

    juce::TextButton playButton{"Play"};
    juce::TextButton stopButton{"Stop"};
    juce::TextButton saveButton{"Save"};
    juce::TextButton loadButton{"Load"};
    juce::Label bpmLabel{"", "BPM"};
    juce::Slider bpmSlider;
    juce::Label positionLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;
    bool fileDragOver = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
