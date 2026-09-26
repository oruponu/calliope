#pragma once

#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/strips/IndexSelection.h"
#include "ui/pianoroll/strips/RangeSelectGesture.h"
#include "ui/pianoroll/strips/TimelineStrip.h"
#include <functional>
#include <variant>
#include <vector>

class TempoTrackStrip : public TimelineStrip
{
public:
    static constexpr int height = 48;

    TempoTrackStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef,
                    juce::UndoManager& undoManagerRef);

    std::function<void()> onSelectionTaken;
    std::function<void()> onTempoChanged;

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
    };
    struct RangeSelecting
    {
        RangeSelectGesture gesture;
    };

    void tempoChanged() override { repaint(); }
    void timelineMetadataChanged() override { repaint(); }

    void deleteSelectedTempoPointsImpl(const juce::String& transactionName);
    float tempoBpmToY(double bpm) const;
    double tempoYToBpm(int y) const;
    int hitTestTempoPoint(int x, int y) const;
    bool hitTestTempoLine(int x, int y, int& outTick, double& outBpm) const;

    EditClipboard& clipboard;
    juce::UndoManager& undoManager;

    IndexSelection selection;
    std::variant<Idle, PointDragging, RangeSelecting> drag;
};
