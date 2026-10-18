#pragma once

#include "ui/theme/Theme.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <utility>

class SegmentedControl : public juce::Component
{
public:
    explicit SegmentedControl(juce::StringArray segmentLabels) : labels(std::move(segmentLabels))
    {
        setMouseClickGrabsKeyboardFocus(false);
    }

    void setSelectedIndex(int index)
    {
        selectedIndex = index;
        repaint();
    }

    int getSelectedIndex() const { return selectedIndex; }

    // Dimmed segments stay clickable.
    void setDimmed(bool shouldDim)
    {
        dimmed = shouldDim;
        repaint();
    }

    std::function<void(int index)> onChange;

    void paint(juce::Graphics& g) override
    {
        using namespace calliope::theme;
        const float alpha = dimmed ? 0.5f : 1.0f;
        const auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        const int count = labels.size();
        const float segmentWidth = bounds.getWidth() / static_cast<float>(count);

        g.saveState();
        juce::Path outline;
        outline.addRoundedRectangle(bounds, radius::r2);
        g.reduceClipRegion(outline);
        g.setFont(font::mono(font::sizeSM));
        for (int i = 0; i < count; ++i)
        {
            const auto segment =
                bounds.withX(bounds.getX() + segmentWidth * static_cast<float>(i)).withWidth(segmentWidth);
            const bool selected = i == selectedIndex;
            if (selected)
            {
                g.setColour(accent::soft.withMultipliedAlpha(alpha));
                g.fillRect(segment);
            }
            g.setColour((selected ? accent::strong : text::t2).withMultipliedAlpha(alpha));
            g.drawText(labels[i], segment, juce::Justification::centred);
        }
        g.restoreState();

        g.setColour(border::strong.withMultipliedAlpha(alpha));
        for (int i = 1; i < count; ++i)
        {
            const float x = bounds.getX() + segmentWidth * static_cast<float>(i);
            g.drawVerticalLine(juce::roundToInt(x), bounds.getY(), bounds.getBottom());
        }
        g.drawRoundedRectangle(bounds, radius::r2, 1.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const int count = labels.size();
        if (count == 0 || getWidth() <= 0)
            return;
        const int index = juce::jlimit(0, count - 1, e.x * count / getWidth());
        if (index == selectedIndex)
            return;
        setSelectedIndex(index);
        if (onChange)
            onChange(index);
    }

private:
    juce::StringArray labels;
    int selectedIndex = 0;
    bool dimmed = false;
};
