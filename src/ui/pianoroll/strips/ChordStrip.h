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
#include <variant>
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
    struct Idle
    {
    };
    struct Moving
    {
        int index = -1;
        std::vector<ChordChange> before;
        int grabOffset = 0;
        std::vector<int> group;
    };
    struct EndResizing
    {
        int index = -1;
        std::vector<ChordChange> before;
        int grabOffset = 0;
        std::set<int> selectionBefore;
    };
    struct StartResizing
    {
        int index = -1;
        std::vector<ChordChange> before;
        int grabOffset = 0;
        std::set<int> selectionBefore;
    };
    using EdgeDrag = std::variant<EndResizing, StartResizing>;
    struct JointDragging
    {
        int leftIndex = -1;
        ResizeEdge initialEdge = ResizeEdge::None;
        std::vector<ChordChange> before;
        std::set<int> selectionBefore;
        EdgeDrag edge;
    };
    struct RangeSelecting
    {
        RangeSelectGesture gesture;
        int toggleIndex = -1;
    };
    using DragState = std::variant<Idle, Moving, EndResizing, StartResizing, JointDragging, RangeSelecting>;
    int spanHeight() const { return getHeight() - spanTop * 2; }

    juce::Rectangle<int> chordSpanRect(int index) const;
    juce::Rectangle<int> chordDraftSpanRect(const ChordDraft& draft) const;
    int hitTestChordSpan(int x, int y) const;
    std::pair<int, ResizeEdge> hitTestChordEdge(int x, int y) const;
    bool isJointChordEdge(int index, ResizeEdge edge) const;
    EdgeDrag makeEdgeDrag(const std::vector<ChordChange>& changes, int index, ResizeEdge edge, int grabX,
                          const std::set<int>& selectionBefore) const;
    static DragState fromEdgeDrag(EdgeDrag edge);
    void switchJointChordEdge(JointDragging& joint, int x, int grabX) const;
    void dragEdge(const EndResizing& resizing, int x);
    void dragEdge(const StartResizing& resizing, int x);
    void remapSelectionAfterResize(const std::vector<ChordChange>& before, int draggedIndex, ResizeEdge edge,
                                   const std::set<int>& selectionBefore);
    void selectMovedChords(const Moving& moving, int cursorTick);
    void deleteSelectedChordsImpl(const juce::String& transactionName);
    void openChordEditor(int tick, int endTick, int chordRoot, int chordType, int bassRoot, bool isNew,
                         juce::Rectangle<int> anchorInLocal);
    void commitChordEdit(int chordRoot, int chordType, int bassRoot);
    void cancelChordEdit();

    juce::UndoManager& undoManager;
    EditClipboard& clipboard;

    IndexSelection selection;
    CalloutEditorSession<ChordEditor, ChordDraft> editSession;
    DragState drag;
};
