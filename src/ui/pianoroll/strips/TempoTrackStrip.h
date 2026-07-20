#pragma once

#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/strips/TimelineStrip.h"
#include <functional>
#include <set>
#include <vector>

class TempoTrackStrip : public TimelineStrip
{
public:
    static constexpr int height = 48;

    TempoTrackStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef);

    std::function<void()> onSelectionTaken;
    std::function<void()> onTempoChanged;

    void setUndoManager(juce::UndoManager* um);
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
    void tempoChanged() override { repaint(); }
    void timelineMetadataChanged() override { repaint(); }

    void deleteSelectedTempoPointsImpl(const juce::String& transactionName);
    void drawTempoRangeSelection(juce::Graphics& g);
    float tempoBpmToY(double bpm) const;
    double tempoYToBpm(int y) const;
    int hitTestTempoPoint(int x, int y) const;
    bool hitTestTempoLine(int x, int y, int& outTick, double& outBpm) const;

    EditClipboard& clipboard;
    juce::UndoManager* undoManager = nullptr;

    std::set<int> selectedTempoIndices;
    bool isTempoPointDragging = false;
    int tempoDragIndex = -1;
    bool tempoDragMoved = false;
    std::vector<TempoChange> tempoDragBefore;
    std::vector<int> tempoDragGroup;
    bool isTempoRangeSelecting = false;
    int tempoSelectStartX = 0;
    int tempoSelectCurrentX = 0;
    std::set<int> tempoSelectBase;
};
