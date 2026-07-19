#pragma once

#include "ui/WheelLabel.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class TimeSignatureEditor : public juce::Component
{
public:
    TimeSignatureEditor(int numerator, int denominator, bool startNumeratorEdit);
    ~TimeSignatureEditor() override;

    std::function<void(int num, int den)> onDraftChanged;
    std::function<void(int num, int den)> onCommit;
    std::function<void()> onCancel;

    void abandon();

    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void parentHierarchyChanged() override;

private:
    static int clampNumerator(int value);
    static int snapDenominator(int value);
    void setDraft(int num, int den);
    void refreshLabels();
    void finalize(bool commit);
    void dismissBox();

    int draftNum;
    int draftDen;
    bool startNumeratorEdit;
    bool finalized = false;
    bool initialFocusPending = true;

    WheelLabel numLabel, denLabel;
    juce::Label slashLabel;
};
