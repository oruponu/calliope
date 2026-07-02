#pragma once

#include "Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class Divider : public juce::Component
{
public:
    enum Orientation
    {
        Horizontal,
        Vertical
    };

    explicit Divider(Orientation o = Horizontal) : orientation(o)
    {
        setMouseCursor(o == Horizontal ? juce::MouseCursor::UpDownResizeCursor
                                       : juce::MouseCursor::LeftRightResizeCursor);
    }
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    std::function<void()> onDragStart;
    std::function<void(int delta)> onDrag;

private:
    Orientation orientation;
};

inline void Divider::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    g.setColour(border::strong);
    g.fillRect(getLocalBounds());
    g.setColour(text::t4);
    auto cy = getHeight() / 2.0f;
    auto cx = getWidth() / 2.0f;
    for (float d : {-12.0f, -4.0f, 4.0f, 12.0f})
    {
        if (orientation == Horizontal)
            g.fillEllipse(cx + d - 1.0f, cy - 1.0f, 2.0f, 2.0f);
        else
            g.fillEllipse(cx - 1.0f, cy + d - 1.0f, 2.0f, 2.0f);
    }
}

inline void Divider::mouseDown(const juce::MouseEvent&)
{
    if (onDragStart)
        onDragStart();
}

inline void Divider::mouseDrag(const juce::MouseEvent& e)
{
    if (!onDrag)
        return;
    onDrag(orientation == Horizontal ? e.getDistanceFromDragStartY() : e.getDistanceFromDragStartX());
}
