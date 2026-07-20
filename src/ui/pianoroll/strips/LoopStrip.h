#pragma once

#include "ui/pianoroll/strips/TimelineStrip.h"
#include <functional>

class LoopStrip : public TimelineStrip
{
public:
    static constexpr int height = 14;

    explicit LoopStrip(const TimelineGeometry& geometryRef);

    std::function<void(int startTick, int endTick)> onLoopRegionChanged;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    bool isDragging = false;
    int dragStartTick = 0;
};
