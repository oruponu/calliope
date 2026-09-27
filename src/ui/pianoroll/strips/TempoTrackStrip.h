#pragma once

#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/strips/IndexSelection.h"
#include "ui/pianoroll/strips/RangeSelectGesture.h"
#include "ui/pianoroll/strips/TimelineStrip.h"
#include "undo/UndoHistory.h"
#include <functional>
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
    };
    struct RangeSelecting
    {
        RangeSelectGesture gesture;
    };

    void cancelDrag() override;
    const std::vector<TempoChange>& displayedChanges() const;
    void deleteSelectedTempoPointsImpl(const juce::String& transactionName);
    float tempoBpmToY(double bpm) const;
    double tempoYToBpm(int y) const;
    int hitTestTempoPoint(int x, int y) const;
    bool hitTestTempoLine(int x, int y, int& outTick, double& outBpm) const;

    EditClipboard& clipboard;
    UndoHistory& undoHistory;

    IndexSelection selection;
    std::variant<Idle, PointDragging, RangeSelecting> drag;
};
