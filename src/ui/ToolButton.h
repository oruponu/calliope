#pragma once

#include "ui/Theme.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class ToolButton : public juce::Component
{
public:
    enum Type
    {
        EditTool,
        SelectTool
    };
    ToolButton(Type t) : type(t) { setRepaintsOnMouseActivity(true); }
    Type getType() const { return type; }
    void setActive(bool a)
    {
        active = a;
        repaint();
    }
    bool isActive() const { return active; }
    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent& e) override;
    std::function<void()> onClick;

private:
    Type type;
    bool active = false;
};

inline void ToolButton::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    auto bounds = getLocalBounds().toFloat();
    bool hover = isMouseOver();

    juce::Colour boxColour, boxBorder, iconColour;
    if (active)
    {
        boxColour = accent::soft;
        boxBorder = accent::dim;
        iconColour = accent::strong;
    }
    else
    {
        boxColour = hover ? surface::surface3 : surface::surface2;
        boxBorder = hover ? border::strong : border::normal;
        iconColour = hover ? text::t1 : text::t2;
    }

    g.setColour(boxColour);
    g.fillRoundedRectangle(bounds, radius::r2);
    g.setColour(boxBorder);
    g.drawRoundedRectangle(bounds.reduced(0.5f), radius::r2, 1.0f);

    g.setColour(iconColour);

    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    if (type == EditTool)
    {
        float size = bounds.getHeight() * 0.38f;
        juce::Path path;
        path.addLineSegment({cx - size * 0.7f, cy + size * 0.7f, cx + size * 0.3f, cy - size * 0.7f}, size * 0.28f);
        g.fillPath(path);
        juce::Path tip;
        tip.addTriangle(cx - size * 0.7f, cy + size * 0.7f, cx - size * 0.45f, cy + size * 0.55f, cx - size * 0.55f,
                        cy + size * 0.45f);
        g.fillPath(tip);
    }
    else if (type == SelectTool)
    {
        float size = bounds.getHeight() * 0.42f;
        juce::Path arrow;
        arrow.startNewSubPath(cx - size * 0.3f, cy - size * 0.75f);
        arrow.lineTo(cx - size * 0.3f, cy + size * 0.75f);
        arrow.lineTo(cx + size * 0.05f, cy + size * 0.35f);
        arrow.lineTo(cx + size * 0.5f, cy + size * 0.35f);
        arrow.closeSubPath();
        g.fillPath(arrow);
    }
}

inline void ToolButton::mouseUp(const juce::MouseEvent& e)
{
    if (active)
        return;
    if (getLocalBounds().contains(e.getPosition()) && onClick)
        onClick();
}
