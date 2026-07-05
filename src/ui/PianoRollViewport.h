#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class PianoRollViewport : public juce::Viewport
{
public:
    PianoRollViewport() { getVerticalScrollBar().addComponentListener(&scrollBarInsetListener); }

    ~PianoRollViewport() override { getVerticalScrollBar().removeComponentListener(&scrollBarInsetListener); }

    void setVerticalScrollBarBottomInset(int inset)
    {
        scrollBarBottomInset = inset;
        applyScrollBarInset();
    }

    std::function<void()> onReachedEnd;
    std::function<void()> onVisibleAreaChanged;
    std::function<void(const juce::MouseEvent&, const juce::MouseWheelDetails&)> onZoom;

    void visibleAreaChanged(const juce::Rectangle<int>&) override
    {
        if (onVisibleAreaChanged)
            onVisibleAreaChanged();
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        if (e.mods.isCommandDown() && onZoom)
        {
            onZoom(e, wheel);
            return;
        }
        if (auto* content = getViewedComponent())
        {
            int speed = 600;
            int newX = getViewPositionX() - juce::roundToInt(wheel.deltaX * speed);
            int newY = getViewPositionY() - juce::roundToInt(wheel.deltaY * speed);
            newX = juce::jlimit(0, juce::jmax(0, content->getWidth() - getViewWidth()), newX);
            newY = juce::jlimit(0, juce::jmax(0, content->getHeight() - getViewHeight()), newY);
            setViewPosition(newX, newY);
        }
        if (isAtRightEdge() && onReachedEnd)
            onReachedEnd();
    }

private:
    bool isAtRightEdge() const
    {
        if (auto* content = getViewedComponent())
            return getViewPositionX() + getViewWidth() >= content->getWidth() - 1;
        return false;
    }

    void applyScrollBarInset()
    {
        auto& vbar = getVerticalScrollBar();
        const bool hBarVisible = getHorizontalScrollBar().isVisible();
        const int fullHeight = getHeight() - (hBarVisible ? getScrollBarThickness() : 0);
        const int target = juce::jlimit(0, fullHeight, fullHeight - scrollBarBottomInset);
        if (vbar.getHeight() != target)
            vbar.setSize(vbar.getWidth(), target);
    }

    struct ScrollBarInsetListener : public juce::ComponentListener
    {
        PianoRollViewport& owner;
        explicit ScrollBarInsetListener(PianoRollViewport& o) : owner(o) {}
        void componentMovedOrResized(juce::Component&, bool, bool wasResized) override
        {
            if (wasResized)
                owner.applyScrollBarInset();
        }
    };

    ScrollBarInsetListener scrollBarInsetListener{*this};
    int scrollBarBottomInset = 0;
};
