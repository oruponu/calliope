#pragma once

#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/strips/TimeSignatureEditor.h"
#include "ui/pianoroll/strips/TimelineStrip.h"
#include <functional>
#include <set>
#include <vector>

class TimeSignatureStrip : public TimelineStrip
{
public:
    static constexpr int height = 24;

    TimeSignatureStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef);
    ~TimeSignatureStrip() override;

    std::function<void()> onSelectionTaken;
    std::function<void()> onTimelineMetadataChanged;

    void setUndoManager(juce::UndoManager* um);
    void setSequence(MidiSequence* seq) override;
    bool hasSelection() const;
    void clearTimeSignatureSelection();
    void deleteSelectedTimeSignatures();
    void copySelectedTimeSignatures();
    void cutSelectedTimeSignatures();
    void pasteTimeSignatures(int atTick);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    void timelineMetadataChanged() override { repaint(); }

    int hitTestTimeSignaturePoint(int x, int y) const;
    juce::Rectangle<int> timeSignatureLabelRect(int index) const;
    void drawTimeSignatureRangeSelection(juce::Graphics& g);
    void deleteSelectedTimeSignaturesImpl(const juce::String& transactionName);
    void openTimeSignatureEditor(int tick, int num, int den, bool isNew, juce::Rectangle<int> anchorInLocal);
    void commitTimeSignatureEdit(int num, int den);
    void cancelTimeSignatureEdit();
    void closeTimeSignatureEditor();

    EditClipboard& clipboard;
    juce::UndoManager* undoManager = nullptr;

    std::set<int> selectedTimeSigIndices;
    bool isTimeSigEditing = false;
    int timeSigEditTick = 0;
    int timeSigDraftNum = 4;
    int timeSigDraftDen = 4;
    bool timeSigEditIsNew = false;
    juce::Component::SafePointer<juce::CallOutBox> timeSigCallout;
    juce::Component::SafePointer<TimeSignatureEditor> timeSigEditor;
    bool isTimeSigPointDragging = false;
    int timeSigDragIndex = -1;
    bool timeSigDragMoved = false;
    std::vector<TimeSignatureChange> timeSigDragBefore;
    int timeSigDragGrabOffset = 0;
    std::vector<int> timeSigDragGroup;
    bool isTimeSigRangeSelecting = false;
    int timeSigSelectStartX = 0;
    int timeSigSelectCurrentX = 0;
    std::set<int> timeSigSelectBase;
};
