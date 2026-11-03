#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace calliope
{
// A desktop window kept for reuse that draws a drop shadow behind one desktop window at a time.
class MenuShadowWindow : public juce::Component, private juce::ComponentListener
{
public:
    MenuShadowWindow();
    ~MenuShadowWindow() override;

    bool isAttached() const;
    bool isAttachedTo(const juce::Component& component) const;
    void attachTo(juce::Component& component);

    void paint(juce::Graphics&) override;
    float getDesktopScaleFactor() const override;

private:
    void detach();
    void followTarget();

    void componentMovedOrResized(juce::Component&, bool wasMoved, bool wasResized) override;
    void componentVisibilityChanged(juce::Component&) override;
    void componentBroughtToFront(juce::Component&) override;
    void componentBeingDeleted(juce::Component&) override;

    juce::WeakReference<juce::Component> target;
    bool isFollowing = false;
};
} // namespace calliope
