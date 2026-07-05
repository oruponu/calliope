#pragma once

#include "Theme.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class ZoomStrip : public juce::Component
{
public:
    enum Orientation
    {
        Horizontal,
        Vertical
    };

    explicit ZoomStrip(Orientation o) : orientation(o)
    {
        setRepaintsOnMouseActivity(true);
        addAndMakeVisible(slider);
        slider.setSliderStyle(o == Horizontal ? juce::Slider::LinearHorizontal : juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;

    juce::Slider slider;
    std::function<void()> onZoomIn;
    std::function<void()> onZoomOut;

private:
    Orientation orientation;
    juce::Rectangle<int> minusBounds, plusBounds;
};

inline void ZoomStrip::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.setColour(surface::bg2);
    g.fillRect(getLocalBounds());

    auto drawSymbol = [&](const juce::Rectangle<int>& bounds, bool isMinus)
    {
        bool hover = bounds.contains(getMouseXYRelative());
        float alpha = hover ? 0.9f : 0.5f;
        g.setColour(text::t1.withAlpha(alpha));
        auto cx = bounds.getCentreX();
        auto cy = bounds.getCentreY();
        int halfLen = 4;
        g.drawHorizontalLine(cy, static_cast<float>(cx - halfLen), static_cast<float>(cx + halfLen + 1));
        if (!isMinus)
            g.drawVerticalLine(cx, static_cast<float>(cy - halfLen), static_cast<float>(cy + halfLen + 1));
    };
    drawSymbol(minusBounds, true);
    drawSymbol(plusBounds, false);
}

inline void ZoomStrip::resized()
{
    auto area = getLocalBounds();
    if (orientation == Horizontal)
    {
        int btnSize = area.getHeight();
        minusBounds = area.removeFromLeft(btnSize);
        plusBounds = area.removeFromRight(btnSize);
    }
    else
    {
        int btnSize = area.getWidth();
        minusBounds = area.removeFromTop(btnSize);
        plusBounds = area.removeFromBottom(btnSize);
    }
    slider.setBounds(area);
}

inline void ZoomStrip::mouseUp(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    if (minusBounds.contains(pos) && onZoomOut)
        onZoomOut();
    else if (plusBounds.contains(pos) && onZoomIn)
        onZoomIn();
}
