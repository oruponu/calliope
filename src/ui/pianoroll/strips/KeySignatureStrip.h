#pragma once

#include "ui/pianoroll/EditClipboard.h"
#include "ui/pianoroll/strips/KeySignatureEditor.h"
#include "ui/pianoroll/strips/TimelineStrip.h"
#include <functional>
#include <set>
#include <vector>

class KeySignatureStrip : public TimelineStrip
{
public:
    static constexpr int height = 24;

    KeySignatureStrip(const TimelineGeometry& geometryRef, EditClipboard& clipboardRef);
    ~KeySignatureStrip() override;

    std::function<void()> onSelectionTaken;

    void setUndoManager(juce::UndoManager* um);
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
    void timelineMetadataChanged() override { repaint(); }

    int hitTestKeySignaturePoint(int x, int y) const;
    juce::Rectangle<int> keySignatureLabelRect(int index) const;
    void drawKeySignatureRangeSelection(juce::Graphics& g);
    void deleteSelectedKeySignaturesImpl(const juce::String& transactionName);
    void openKeySignatureEditor(int tick, int sharpsOrFlats, bool isMinor, bool isNew,
                                juce::Rectangle<int> anchorInLocal);
    void commitKeySignatureEdit(int sharpsOrFlats, bool isMinor);
    void cancelKeySignatureEdit();
    void closeKeySignatureEditor();

    EditClipboard& clipboard;
    juce::UndoManager* undoManager = nullptr;

    std::set<int> selectedKeySigIndices;
    bool isKeySigEditing = false;
    int keySigEditTick = 0;
    int keySigDraftSharpsOrFlats = 0;
    bool keySigDraftIsMinor = false;
    bool keySigEditIsNew = false;
    juce::Component::SafePointer<juce::CallOutBox> keySigCallout;
    juce::Component::SafePointer<KeySignatureEditor> keySigEditor;
    bool isKeySigPointDragging = false;
    int keySigDragIndex = -1;
    bool keySigDragMoved = false;
    std::vector<KeySignatureChange> keySigDragBefore;
    int keySigDragGrabOffset = 0;
    std::vector<int> keySigDragGroup;
    bool isKeySigRangeSelecting = false;
    int keySigSelectStartX = 0;
    int keySigSelectCurrentX = 0;
    std::set<int> keySigSelectBase;
};
