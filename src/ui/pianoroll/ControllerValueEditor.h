#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class ControllerValueEditor : public juce::Component
{
public:
    ControllerValueEditor(int displayedValue, int minDisplayed, int maxDisplayed);
    ~ControllerValueEditor() override;

    std::function<void(int displayedValue)> onCommit;
    std::function<void()> onCancel;

    void abandon();

    void resized() override;
    void parentHierarchyChanged() override;

private:
    void finalize(bool commit);
    void dismissBox();

    int minDisplayed;
    int maxDisplayed;
    bool finalized = false;
    bool initialFocusPending = true;

    juce::TextEditor valueEditor;
};
