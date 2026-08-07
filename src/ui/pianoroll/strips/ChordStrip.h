#pragma once

#include "ui/pianoroll/strips/ChordEditor.h"
#include "ui/pianoroll/strips/TimelineStrip.h"
#include <functional>

class ChordStrip : public TimelineStrip
{
public:
    static constexpr int height = 24;

    explicit ChordStrip(const TimelineGeometry& geometryRef);
    ~ChordStrip() override;

    std::function<void()> onSelectionTaken;

    void setUndoManager(juce::UndoManager* um);
    void setSequence(MidiSequence* seq) override;
    void clearChordSelection();

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    void timelineMetadataChanged() override { repaint(); }

    static constexpr int spanTop = 3;
    int spanHeight() const { return getHeight() - spanTop * 2; }

    juce::Rectangle<int> chordSpanRect(int index) const;
    juce::Rectangle<int> chordDraftSpanRect() const;
    int hitTestChordSpan(int x, int y) const;
    void openChordEditor(int tick, int endTick, int chordRoot, int chordType, int bassRoot, bool isNew,
                         juce::Rectangle<int> anchorInLocal);
    void commitChordEdit(int chordRoot, int chordType, int bassRoot);
    void cancelChordEdit();
    void closeChordEditor();

    juce::UndoManager* undoManager = nullptr;

    int selectedChordIndex = -1;
    bool isChordEditing = false;
    int chordEditTick = 0;
    int chordEditEndTick = 0;
    int chordDraftRoot = 0;
    int chordDraftType = 0;
    int chordDraftBassRoot = 0;
    bool chordEditIsNew = false;
    ChordSpelling chordEditSpelling = ChordSpelling::Mixed;
    juce::Component::SafePointer<juce::CallOutBox> chordCallout;
    juce::Component::SafePointer<ChordEditor> chordEditor;
};
