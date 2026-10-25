#pragma once

#include "ui/pianoroll/ProgramBrowserState.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class ProgramBrowser : public juce::Component
{
public:
    explicit ProgramBrowser(int program);
    ~ProgramBrowser() override;

    std::function<void(int program)> onCommit;
    std::function<void()> onCancel;

    void abandon();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void parentHierarchyChanged() override;

private:
    void finalize(bool commit);
    void dismissBox();

    ProgramBrowserState state;
    bool finalized = false;
    bool initialFocusPending = true;
    juce::Rectangle<int> familyBounds;
    juce::Rectangle<int> programBounds;
};
