#pragma once

#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/strips/CalloutEditorSession.h"
#include "ui/pianoroll/strips/IndexSelection.h"
#include "ui/pianoroll/strips/KeySignatureEditor.h"
#include "ui/pianoroll/strips/RangeSelectGesture.h"
#include "ui/pianoroll/strips/TimelineStrip.h"
#include <functional>
#include <variant>
#include <vector>

class KeySignatureStrip : public TimelineStrip
{
public:
    static constexpr int height = 24;

    KeySignatureStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef,
                      juce::UndoManager& undoManagerRef);

    std::function<void()> onSelectionTaken;

    void setSequence(MidiSequence* seq) override;
    bool hasSelection() const;
    void clearKeySignatureSelection();
    void deleteSelectedKeySignatures();
    void copySelectedKeySignatures();
    void cutSelectedKeySignatures();
    void pasteKeySignatures(int atTick);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    struct KeySignatureDraft
    {
        int tick = 0;
        int sharpsOrFlats = 0;
        bool isMinor = false;
        bool isNew = false;
    };
    struct Idle
    {
    };
    struct PointDragging
    {
        int index = -1;
        std::vector<KeySignatureChange> before;
        int grabOffset = 0;
        std::vector<int> group;
        bool moved = false;
    };
    struct RangeSelecting
    {
        RangeSelectGesture gesture;
    };

    void timelineMetadataChanged() override { repaint(); }

    int hitTestKeySignaturePoint(int x, int y) const;
    juce::Rectangle<int> keySignatureLabelRect(int index) const;
    void deleteSelectedKeySignaturesImpl(const juce::String& transactionName);
    void openKeySignatureEditor(int tick, int sharpsOrFlats, bool isMinor, bool isNew,
                                juce::Rectangle<int> anchorInLocal);
    void commitKeySignatureEdit(int sharpsOrFlats, bool isMinor);
    void cancelKeySignatureEdit();

    EditClipboard& clipboard;
    juce::UndoManager& undoManager;

    IndexSelection selection;
    CalloutEditorSession<KeySignatureEditor, KeySignatureDraft> editSession;
    std::variant<Idle, PointDragging, RangeSelecting> drag;
};
