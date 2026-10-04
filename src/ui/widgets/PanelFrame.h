#pragma once

#include "ui/theme/Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

// JUCE cannot clip child components to a rounded shape, so the corners are masked from above instead.
class PanelFrame : public juce::Component
{
public:
    PanelFrame()
    {
        setInterceptsMouseClicks(false, false);
        setOpaque(false);
        setAlwaysOnTop(true);
    }
    void paint(juce::Graphics& g) override;
    void setFocused(bool shouldBeFocused);

private:
    bool focused = false;
};

inline void PanelFrame::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    auto bounds = getLocalBounds().toFloat();

    juce::Path corners;
    corners.setUsingNonZeroWinding(false);
    corners.addRectangle(bounds);
    corners.addRoundedRectangle(bounds, radius::r3);
    g.setColour(surface::bg);
    g.fillPath(corners);

    g.setColour(focused ? text::t2 : border::normal);
    g.drawRoundedRectangle(bounds.reduced(0.5f), radius::r3, 1.0f);
}

inline void PanelFrame::setFocused(bool shouldBeFocused)
{
    if (focused == shouldBeFocused)
        return;
    focused = shouldBeFocused;
    repaint();
}
