#pragma once

#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/strips/IndexSelection.h"
#include "ui/pianoroll/strips/RangeSelectGesture.h"
#include "ui/pianoroll/strips/TimelineStrip.h"
#include "undo/UndoHistory.h"
#include <functional>
#include <optional>
#include <variant>
#include <vector>

class TempoTrackStrip : public TimelineStrip
{
public:
    static constexpr int height = 48;

    TempoTrackStrip(const TimelineGeometry& geometryRef, const DisplayedTimeline& displayedTimelineRef,
                    EditClipboard& clipboardRef, UndoHistory& undoHistoryRef);

    std::function<void()> onSelectionTaken;

    void setSequence(MidiSequence* seq) override;
    bool hasSelection() const;
    void clearTempoSelection();
    void deleteSelectedTempoPoints();
    void copySelectedTempoPoints();
    void cutSelectedTempoPoints();
    void pasteTempoPoints(int atTick);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

private:
    struct Idle
    {
    };
    struct PointDragging
    {
        int index = -1;
        std::vector<TempoChange> before;
        std::vector<int> group;
        bool moved = false;
        std::vector<TempoChange> preview;
        // Where the mouse is once the drag has moved, for the drag tip.
        std::optional<juce::Point<int>> tipMouse;
        // Set when the point was added on the line by this press; the add is applied with the drag as one edit.
        std::optional<std::vector<TempoChange>> beforeAdd;
    };
    struct RangeSelecting
    {
        RangeSelectGesture gesture;
    };

    void cancelDrag() override;
    const std::vector<TempoChange>& displayedChanges() const;
    void drawDragValueTip(juce::Graphics& g);
    float tempoBpmToY(double bpm) const;
    double tempoYToBpm(int y) const;
    int hitTestTempoPoint(int x, int y) const;
    bool hitTestTempoLine(int x, int y, int& outTick, double& outBpm) const;
    void deleteSelectedTempoPointsImpl(const juce::String& transactionName);

    EditClipboard& clipboard;
    UndoHistory& undoHistory;

    IndexSelection selection;
    // The point the first click of a double-click pressed, so a point that click added is not deleted.
    int doubleClickPoint = -1;
    std::variant<Idle, PointDragging, RangeSelecting> drag;
};
