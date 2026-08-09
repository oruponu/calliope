#pragma once

#include "ui/pianoroll/strips/ChordEditor.h"
#include "ui/pianoroll/strips/TimelineStrip.h"
#include <functional>
#include <utility>
#include <vector>

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
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    void timelineMetadataChanged() override { repaint(); }

    static constexpr int spanTop = 3;
    static constexpr int resizeEdgeWidth = 6;

    enum class ResizeEdge
    {
        None,
        Left,
        Right
    };
    int spanHeight() const { return getHeight() - spanTop * 2; }

    juce::Rectangle<int> chordSpanRect(int index) const;
    juce::Rectangle<int> chordDraftSpanRect() const;
    int hitTestChordSpan(int x, int y) const;
    std::pair<int, ResizeEdge> hitTestChordEdge(int x, int y) const;
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
    bool isChordResizing = false;
    int chordResizeIndex = -1;
    std::vector<ChordChange> chordResizeBefore;
    int chordResizeGrabOffset = 0;
    bool isChordMoving = false;
    int chordMoveIndex = -1;
    std::vector<ChordChange> chordMoveBefore;
    int chordMoveGrabOffset = 0;
    bool isChordStartResizing = false;
    int chordStartResizeIndex = -1;
    std::vector<ChordChange> chordStartResizeBefore;
    int chordStartResizeGrabOffset = 0;
};
