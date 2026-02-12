#pragma once

#include "audio/MidiDeviceOutput.h"
#include "engine/PlaybackEngine.h"
#include "model/MidiSequence.h"
#include "ui/PianoRollComponent.h"
#include "ui/TrackListComponent.h"
#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent : public juce::Component, public juce::FileDragAndDropTarget
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
    class TransportButton : public juce::Component
    {
    public:
        enum Type
        {
            Stop,
            Play,
            Pause
        };
        TransportButton(Type t) : type(t)
        {
            setRepaintsOnMouseActivity(true);
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        }
        void setType(Type t)
        {
            type = t;
            repaint();
        }
        Type getType() const { return type; }
        void paint(juce::Graphics& g) override;
        void mouseUp(const juce::MouseEvent& e) override;
        std::function<void()> onClick;

    private:
        Type type;
    };

    void onVBlank();
    void buildTestSequence();
    void updateTransportDisplay();
    void saveFile();
    void loadFile();
    void onSequenceLoaded();

    MidiSequence sequence;
    PlaybackEngine playbackEngine;
    MidiDeviceOutput midiOutput;

    PianoRollComponent pianoRoll;
    juce::Viewport viewport;
    TrackListComponent trackList;
    juce::Viewport trackListViewport;

    TransportButton stopButton{TransportButton::Stop};
    TransportButton playButton{TransportButton::Play};

    juce::TextButton saveButton{"Save"};
    juce::TextButton loadButton{"Load"};

    juce::Label positionHeaderLabel{"", "Bar"};
    juce::Label positionLabel;

    juce::Label timeSigHeaderLabel{"", "Beat"};
    juce::Label timeSigValueLabel;

    juce::Label keyHeaderLabel{"", "Key"};
    juce::Label keyValueLabel;

    juce::Label tempoHeaderLabel{"", "Tempo"};
    juce::Label tempoValueLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::VBlankAttachment> vblankAttachment;
    bool fileDragOver = false;

    static constexpr int transportBarHeight = 64;
    static constexpr int trackListWidth = 180;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
