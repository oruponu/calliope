#pragma once

#include "Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

class FocusBorder : public juce::Component
{
public:
    FocusBorder()
    {
        setInterceptsMouseClicks(false, false);
        setOpaque(false);
        setAlwaysOnTop(true);
    }
    void paint(juce::Graphics& g) override;
};

inline void FocusBorder::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.setColour(text::t2);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), radius::r2, 1.0f);
}
