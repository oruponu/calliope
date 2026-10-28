#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

// Reports Tab and Shift+Tab instead of letting them move the keyboard focus to another component.
class TrackNameEditor : public juce::TextEditor
{
public:
    std::function<void(bool backwards)> onTabKey;

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key.getKeyCode() == juce::KeyPress::tabKey && onTabKey)
        {
            onTabKey(key.getModifiers().isShiftDown());
            return true;
        }
        return juce::TextEditor::keyPressed(key);
    }
};
