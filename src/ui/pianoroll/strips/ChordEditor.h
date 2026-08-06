#pragma once

#include "model/MidiSequence.h"
#include "ui/widgets/WheelLabel.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class ChordEditor : public juce::Component
{
public:
    ChordEditor(int chordRoot, int chordType, int bassRoot, ChordSpelling spelling, bool startRootEdit);
    ~ChordEditor() override;

    std::function<void(int chordRoot, int chordType, int bassRoot)> onDraftChanged;
    std::function<void(int chordRoot, int chordType, int bassRoot)> onCommit;
    std::function<void()> onCancel;

    void abandon();

    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void parentHierarchyChanged() override;

private:
    void setDraft(int chordRoot, int chordType, int bassRoot);
    void nudgeRoot(int direction);
    void nudgeType(int direction);
    void nudgeBass(int direction);
    void refreshLabels();
    void finalize(bool commit);
    void dismissBox();

    int draftRoot;
    int draftType;
    int draftBassRoot;
    ChordSpelling spelling;
    bool startRootEdit;
    bool finalized = false;
    bool initialFocusPending = true;

    WheelLabel rootLabel, typeLabel, bassLabel;
    juce::Label slashLabel;
};
