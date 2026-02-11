#pragma once

#include "../model/MidiSequence.h"
#include <juce_gui_basics/juce_gui_basics.h>

class PianoRollComponent : public juce::Component
{
public:
    void setSequence(const MidiSequence* seq);
    void setPlayheadTick(int tick);
    void paint(juce::Graphics& g) override;

    static constexpr int keyboardWidth = 72;
    static constexpr int noteHeight = 14;
    static constexpr int beatWidth = 80;
    static constexpr int totalNotes = 128;
    static constexpr int beatsPerBar = 4;

private:
    void drawKeyboard(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawNotes(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    void updateSize();

    int noteToY(int noteNumber) const;
    int tickToX(int tick) const;
    int tickToWidth(int durationTicks) const;
    int getTotalBeats() const;

    static bool isBlackKey(int noteNumber);
    static juce::String getNoteName(int noteNumber);

    const MidiSequence* sequence = nullptr;
    int playheadTick = 0;
};
