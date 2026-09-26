#pragma once

#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/strips/CalloutEditorSession.h"
#include "ui/pianoroll/strips/IndexSelection.h"
#include "ui/pianoroll/strips/RangeSelectGesture.h"
#include "ui/pianoroll/strips/TimeSignatureEditor.h"
#include "ui/pianoroll/strips/TimelineStrip.h"
#include <functional>
#include <vector>

class TimeSignatureStrip : public TimelineStrip
{
public:
    static constexpr int height = 24;

    TimeSignatureStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef,
                       juce::UndoManager& undoManagerRef);

    std::function<void()> onSelectionTaken;
    std::function<void()> onTimelineMetadataChanged;

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
    struct TimeSignatureDraft
    {
        int tick = 0;
        int numerator = 4;
        int denominator = 4;
        bool isNew = false;
    };

    void timelineMetadataChanged() override { repaint(); }

    int hitTestTimeSignaturePoint(int x, int y) const;
    juce::Rectangle<int> timeSignatureLabelRect(int index) const;
    void deleteSelectedTimeSignaturesImpl(const juce::String& transactionName);
    void openTimeSignatureEditor(int tick, int num, int den, bool isNew, juce::Rectangle<int> anchorInLocal);
    void commitTimeSignatureEdit(int num, int den);
    void cancelTimeSignatureEdit();

    EditClipboard& clipboard;
    juce::UndoManager& undoManager;

    IndexSelection selection;
    CalloutEditorSession<TimeSignatureEditor, TimeSignatureDraft> editSession;
    bool isTimeSigPointDragging = false;
    int timeSigDragIndex = -1;
    bool timeSigDragMoved = false;
    std::vector<TimeSignatureChange> timeSigDragBefore;
    int timeSigDragGrabOffset = 0;
    std::vector<int> timeSigDragGroup;
    bool isTimeSigRangeSelecting = false;
    RangeSelectGesture rangeSelect;
};
