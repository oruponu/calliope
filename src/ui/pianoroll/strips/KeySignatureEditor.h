#pragma once

#include "ui/widgets/WheelLabel.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class KeySignatureEditor : public juce::Component
{
public:
    KeySignatureEditor(int sharpsOrFlats, bool isMinor, bool startTextEdit);
    ~KeySignatureEditor() override;

    std::function<void(int sharpsOrFlats, bool isMinor)> onDraftChanged;
    std::function<void(int sharpsOrFlats, bool isMinor)> onCommit;
    std::function<void()> onCancel;

    void abandon();

    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void parentHierarchyChanged() override;

private:
    void setDraft(int sharpsOrFlats, bool isMinor);
    void nudgeDraft(int direction);
    void refreshLabel();
    void finalize(bool commit);
    void dismissBox();

    int draftSharpsOrFlats;
    bool draftIsMinor;
    bool startTextEdit;
    bool finalized = false;
    bool initialFocusPending = true;

    WheelLabel keyLabel;
};
