#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class WheelLabel : public juce::Label
{
public:
    // direction is +1 (up) or -1 (down)
    std::function<void(int direction)> onWheel;

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        if (onWheel && wheel.deltaY != 0.0f)
            onWheel(wheel.deltaY > 0.0f ? 1 : -1);
        else
            juce::Label::mouseWheelMove(e, wheel);
    }
};
