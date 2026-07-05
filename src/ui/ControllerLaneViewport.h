#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class ControllerLaneViewport : public juce::Viewport
{
public:
    std::function<void()> onVisibleAreaChanged;
    std::function<void()> onReachedEnd;

    ControllerLaneViewport()
    {
        getHorizontalScrollBar().addMouseListener(&scrollBarListener, false);
        getHorizontalScrollBar().addComponentListener(&scrollBarInsetListener);
    }

    ~ControllerLaneViewport() override
    {
        getHorizontalScrollBar().removeComponentListener(&scrollBarInsetListener);
        getHorizontalScrollBar().removeMouseListener(&scrollBarListener);
    }

    void setHorizontalScrollBarRightInset(int inset)
    {
        scrollBarRightInset = inset;
        applyScrollBarInset();
    }

    void visibleAreaChanged(const juce::Rectangle<int>&) override
    {
        pendingExtend = isAtRightEdge();
        if (onVisibleAreaChanged)
            onVisibleAreaChanged();
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
        auto& hbar = getHorizontalScrollBar();
        const bool vBarVisible = getVerticalScrollBar().isVisible();
        const int fullWidth = getWidth() - (vBarVisible ? getScrollBarThickness() : 0);
        const int target = juce::jlimit(0, fullWidth, fullWidth - scrollBarRightInset);
        if (hbar.getWidth() != target)
            hbar.setSize(target, hbar.getHeight());
    }

    struct ScrollBarInsetListener : public juce::ComponentListener
    {
        ControllerLaneViewport& owner;
        explicit ScrollBarInsetListener(ControllerLaneViewport& o) : owner(o) {}
        void componentMovedOrResized(juce::Component&, bool, bool wasResized) override
        {
            if (wasResized)
                owner.applyScrollBarInset();
        }
    };

    ScrollBarInsetListener scrollBarInsetListener{*this};
    int scrollBarRightInset = 0;

    struct ScrollBarListener : public juce::MouseListener
    {
        ControllerLaneViewport& owner;
        explicit ScrollBarListener(ControllerLaneViewport& o) : owner(o) {}
        void mouseUp(const juce::MouseEvent&) override
        {
            if (owner.pendingExtend && owner.onReachedEnd)
            {
                owner.pendingExtend = false;
                owner.onReachedEnd();
            }
        }
    };

    ScrollBarListener scrollBarListener{*this};
    bool pendingExtend = false;
};
