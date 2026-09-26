#pragma once

#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/strips/CalloutEditorSession.h"
#include "ui/pianoroll/strips/ChordEditor.h"
#include "ui/pianoroll/strips/IndexSelection.h"
#include "ui/pianoroll/strips/RangeSelectGesture.h"
#include "ui/pianoroll/strips/TimelineStrip.h"
#include <functional>
#include <set>
#include <utility>
#include <vector>

class ChordStrip : public TimelineStrip
{
public:
    static constexpr int height = 24;

    ChordStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef, juce::UndoManager& undoManagerRef);

    std::function<void()> onSelectionTaken;

    void setSequence(MidiSequence* seq) override;
    void clearChordSelection();
    bool hasSelection() const;
    void deleteSelectedChords();
    void copySelectedChords();
    void cutSelectedChords();
    void pasteChords(int atTick);

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
    struct ChordDraft
    {
        int tick = 0;
        int endTick = 0;
        int root = 0;
        int type = 0;
        int bassRoot = 0;
        bool isNew = false;
    };
    int spanHeight() const { return getHeight() - spanTop * 2; }

    juce::Rectangle<int> chordSpanRect(int index) const;
    juce::Rectangle<int> chordDraftSpanRect(const ChordDraft& draft) const;
    int hitTestChordSpan(int x, int y) const;
    std::pair<int, ResizeEdge> hitTestChordEdge(int x, int y) const;
    bool isJointChordEdge(int index, ResizeEdge edge) const;
    void beginChordEdgeDrag(const std::vector<ChordChange>& changes, int index, ResizeEdge edge, int grabX);
    void switchJointChordEdge(int x, int grabX);
    void remapSelectionAfterResize(const std::vector<ChordChange>& before, int draggedIndex, ResizeEdge edge);
    void selectMovedChords(int anchorIndex, int cursorTick);
    void deleteSelectedChordsImpl(const juce::String& transactionName);
    void openChordEditor(int tick, int endTick, int chordRoot, int chordType, int bassRoot, bool isNew,
                         juce::Rectangle<int> anchorInLocal);
    void commitChordEdit(int chordRoot, int chordType, int bassRoot);
    void cancelChordEdit();

    juce::UndoManager& undoManager;
    EditClipboard& clipboard;

    IndexSelection selection;
    CalloutEditorSession<ChordEditor, ChordDraft> editSession;
    bool isChordResizing = false;
    int chordResizeIndex = -1;
    std::vector<ChordChange> chordResizeBefore;
    int chordResizeGrabOffset = 0;
    bool isChordMoving = false;
    int chordMoveIndex = -1;
    std::vector<ChordChange> chordMoveBefore;
    int chordMoveGrabOffset = 0;
    std::vector<int> chordMoveGroup;
    bool isChordStartResizing = false;
    int chordStartResizeIndex = -1;
    std::vector<ChordChange> chordStartResizeBefore;
    int chordStartResizeGrabOffset = 0;
    bool isChordJointDragging = false;
    int chordJointLeftIndex = -1;
    ResizeEdge chordJointEdge = ResizeEdge::None;
    std::vector<ChordChange> chordJointBefore;
    std::set<int> chordEdgeSelectionBefore;
    bool isChordRangeSelecting = false;
    RangeSelectGesture rangeSelect;
    int chordRangeToggleIndex = -1;
};
