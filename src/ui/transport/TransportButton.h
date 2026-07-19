#pragma once

#include "ui/theme/Theme.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class TransportButton : public juce::Component
{
public:
    enum Type
    {
        ReturnToStart,
        Stop,
        Play,
        Loop
    };
    TransportButton(Type t) : type(t) { setRepaintsOnMouseActivity(true); }
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

inline void TransportButton::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    auto bounds = getLocalBounds().toFloat();
    bool hover = isMouseOver();

    juce::Colour boxColour, boxBorder, iconColour;
    if (type == Play && active)
    {
        boxColour = accent::base;
        boxBorder = accent::base;
        iconColour = surface::bg;
    }
    else if (type == Loop && active)
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

    if (type == ReturnToStart)
    {
        auto h = bounds.getHeight() * 0.45f;
        auto w = h * 0.55f;
        auto cx = bounds.getCentreX();
        auto cy = bounds.getCentreY();
        auto barW = bounds.getWidth() * 0.08f;
        g.fillRoundedRectangle(cx - w * 0.45f - barW, cy - h / 2, barW, h, 1.0f);
        juce::Path path;
        path.addTriangle(cx + w * 0.55f, cy - h / 2, cx + w * 0.55f, cy + h / 2, cx - w * 0.4f, cy);
        g.fillPath(path);
    }
    else if (type == Stop)
    {
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.42f;
        g.fillRoundedRectangle(bounds.withSizeKeepingCentre(size, size), 2.0f);
    }
    else if (type == Play)
    {
        auto h = bounds.getHeight() * 0.5f;
        auto w = h * 0.85f;
        auto cx = bounds.getCentreX();
        auto cy = bounds.getCentreY();
        juce::Path path;
        path.addTriangle(cx - w * 0.38f, cy - h / 2, cx - w * 0.38f, cy + h / 2, cx + w * 0.62f, cy);
        g.fillPath(path);
    }
    else if (type == Loop)
    {
        auto cx = bounds.getCentreX();
        auto cy = bounds.getCentreY();
        float s = bounds.getHeight() * 0.2f;
        float hw = s * 0.6f;
        float hh = s * 0.75f;
        float gap = hw * 0.8f;
        float a = hh * 0.8f;

        juce::Path rp;
        rp.startNewSubPath(cx + gap, cy - hh);
        rp.lineTo(cx + hw, cy - hh);
        rp.quadraticTo(cx + hw + hh, cy - hh, cx + hw + hh, cy);
        rp.quadraticTo(cx + hw + hh, cy + hh, cx + hw, cy + hh);
        rp.lineTo(cx + gap, cy + hh);
        g.strokePath(rp, juce::PathStrokeType(1.5f));

        juce::Path lp;
        lp.startNewSubPath(cx - gap, cy + hh);
        lp.lineTo(cx - hw, cy + hh);
        lp.quadraticTo(cx - hw - hh, cy + hh, cx - hw - hh, cy);
        lp.quadraticTo(cx - hw - hh, cy - hh, cx - hw, cy - hh);
        lp.lineTo(cx - gap, cy - hh);
        g.strokePath(lp, juce::PathStrokeType(1.5f));

        juce::Path la;
        la.addTriangle(cx + gap - a, cy + hh, cx + gap, cy + hh - a, cx + gap, cy + hh + a);
        g.fillPath(la);

        juce::Path ra;
        ra.addTriangle(cx - gap + a, cy - hh, cx - gap, cy - hh - a, cx - gap, cy - hh + a);
        g.fillPath(ra);
    }
}

inline void TransportButton::mouseUp(const juce::MouseEvent& e)
{
    if (active && type != Loop)
        return;
    if (getLocalBounds().contains(e.getPosition()) && onClick)
        onClick();
}
