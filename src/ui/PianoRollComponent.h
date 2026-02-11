#pragma once

#include "../model/MidiSequence.h"
#include <juce_gui_basics/juce_gui_basics.h>

class PianoRollComponent : public juce::Component
{
public:
    void setSequence(MidiSequence* seq);
    void setPlayheadTick(int tick);
    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

    static constexpr int keyboardWidth = 72;
    static constexpr int noteHeight = 14;
    static constexpr int beatWidth = 80;
    static constexpr int totalNotes = 128;
    static constexpr int beatsPerBar = 4;
    static constexpr int snapTicks = 480;
    static constexpr int resizeEdgeWidth = 6;

private:
    enum class DragMode
    {
        None,
        Moving,
        Resizing
    };

    struct NoteRef
    {
        int trackIndex = -1;
        int noteIndex = -1;
        bool isValid() const { return trackIndex >= 0 && noteIndex >= 0; }
    };

    void drawKeyboard(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawNotes(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    void updateSize();

    int noteToY(int noteNumber) const;
    int tickToX(int tick) const;
    int tickToWidth(int durationTicks) const;
    int xToTick(int x) const;
    int yToNote(int y) const;
    int snapTick(int tick) const;
    int getTotalBeats() const;

    NoteRef hitTestNote(int x, int y) const;
    bool isOnRightEdge(int x, const MidiNote& note) const;

    int getKeyboardLeft() const;

    static bool isBlackKey(int noteNumber);
    static juce::String getNoteName(int noteNumber);

    MidiSequence* sequence = nullptr;
    int playheadTick = 0;

    NoteRef selectedNote;
    DragMode dragMode = DragMode::None;
    int dragStartTick = 0;
    int dragStartNote = 0;
    int originalStartTick = 0;
    int originalNoteNumber = 0;
    int originalDuration = 0;
};
