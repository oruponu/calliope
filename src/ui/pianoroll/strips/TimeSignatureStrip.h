#pragma once

#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/strips/CalloutEditorSession.h"
#include "ui/pianoroll/strips/IndexSelection.h"
#include "ui/pianoroll/strips/RangeSelectGesture.h"
#include "ui/pianoroll/strips/TimeSignatureEditor.h"
#include "ui/pianoroll/strips/TimelineStrip.h"
#include "undo/UndoHistory.h"
#include <functional>
#include <variant>
#include <vector>

class TimeSignatureStrip : public TimelineStrip
{
public:
    static constexpr int height = 24;

    TimeSignatureStrip(const TimelineGeometry& geometryRef, const DisplayedTimeline& displayedTimelineRef,
                       EditClipboard& clipboardRef, UndoHistory& undoHistoryRef);

    std::function<void()> onSelectionTaken;
    std::function<void(const std::vector<TimeSignatureChange>* preview)> onTimeSignaturePreview;

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
    struct Idle
    {
    };
    struct PointDragging
    {
        int index = -1;
        std::vector<TimeSignatureChange> before;
        int grabOffset = 0;
        std::vector<int> group;
        bool moved = false;
        std::vector<TimeSignatureChange> preview;
    };
    struct RangeSelecting
    {
        RangeSelectGesture gesture;
    };

    void cancelDrag() override;
    const std::vector<TimeSignatureChange>& displayedChanges() const;
    juce::Rectangle<int> timeSignatureLabelRect(int index) const;
    int hitTestTimeSignaturePoint(int x, int y) const;
    void deleteSelectedTimeSignaturesImpl(const juce::String& transactionName);
    void openTimeSignatureEditor(int tick, int num, int den, bool isNew, juce::Rectangle<int> anchorInLocal);
    void commitTimeSignatureEdit(int num, int den);
    void cancelTimeSignatureEdit();

    EditClipboard& clipboard;
    UndoHistory& undoHistory;

    IndexSelection selection;
    CalloutEditorSession<TimeSignatureEditor, TimeSignatureDraft> editSession;
    std::variant<Idle, PointDragging, RangeSelecting> drag;
};
