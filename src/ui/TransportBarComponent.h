#pragma once

#include "../model/MidiSequence.h"
#include "TransportButton.h"
#include "WheelLabel.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class Document;
class PlaybackEngine;

class TransportBarComponent : public juce::Component, public MidiSequence::Listener
{
public:
    TransportBarComponent(Document& document, PlaybackEngine& playbackEngine);
    ~TransportBarComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Called by MainComponent
    void updateDisplay();
    void setPlaying(bool playing);
    void setLoopActive(bool active);
    void togglePlay();
    void toggleLoop();
    void returnToStart();
    void jumpToTick(int tick);

    // Emitted to MainComponent (effects that reach other components / resources)
    std::function<void(double tick)> onPlayheadMoved;
    std::function<void(int tick)> onScrollToPlayhead;
    std::function<void()> onReturnToStart;
    std::function<void(bool playing)> onPlaybackStateChanged;
    std::function<void(bool enabled, int startTick, int endTick)> onLoopRegionChanged;

private:
    void tempoChanged() override;
    void timelineMetadataChanged() override;

    enum class PositionUnit
    {
        Bar,
        Beat,
        Tick
    };

    enum class TimeSigUnit
    {
        Numerator,
        Denominator
    };

    void stopAndNotify();
    void commitPositionEdit();
    void nudgePosition(PositionUnit unit, int direction);
    void commitTempoEdit();
    void nudgeTempo(int direction);
    void setTempoAtPlayhead(double bpm);
    void commitTimeSignatureEdit();
    void nudgeTimeSignature(TimeSigUnit unit, int direction);
    void setTimeSignatureAtPlayhead(int numerator, int denominator);
    void commitKeySignatureEdit();
    void nudgeKeySignature(int direction);
    void setKeySignatureAtPlayhead(int sharpsOrFlats, bool isMinor);

    Document& document;
    PlaybackEngine& playbackEngine;

    TransportButton returnToStartButton{TransportButton::ReturnToStart};
    TransportButton stopButton{TransportButton::Stop};
    TransportButton playButton{TransportButton::Play};
    TransportButton loopButton{TransportButton::Loop};

    juce::Label positionHeaderLabel{"", "POSITION"};
    WheelLabel positionBarLabel;
    WheelLabel positionBeatLabel;
    WheelLabel positionTickLabel;
    juce::Label positionDot1{"", "."};
    juce::Label positionDot2{"", "."};

    juce::Label timeSigHeaderLabel{"", "TIME"};
    WheelLabel timeSigNumLabel;
    WheelLabel timeSigDenLabel;
    juce::Label timeSigSlashLabel{"", "/"};

    juce::Label keyHeaderLabel{"", "KEY"};
    WheelLabel keyValueLabel;

    juce::Label tempoHeaderLabel{"", "TEMPO"};
    WheelLabel tempoValueLabel;

    juce::Rectangle<int> positionBoxBounds;
    juce::Rectangle<int> infoBoxBounds;
    int infoDividerX1 = 0;
    int infoDividerX2 = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBarComponent)
};
