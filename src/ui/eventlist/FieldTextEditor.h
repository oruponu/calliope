#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

// Reports the wheel and Tab instead of scrolling or moving the keyboard focus to another component.
class FieldTextEditor : public juce::TextEditor
{
public:
    // direction is +1 (up) or -1 (down)
    std::function<void(int direction)> onWheel;
    std::function<void(bool backwards)> onTabKey;

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        if (onWheel && wheel.deltaY != 0.0f)
            onWheel(wheel.deltaY > 0.0f ? 1 : -1);
        else
            juce::TextEditor::mouseWheelMove(e, wheel);
    }

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
