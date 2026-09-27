#pragma once

#include "ui/pianoroll/strips/TimelineStrip.h"
#include <functional>

class RulerStrip : public TimelineStrip
{
public:
    static constexpr int height = 24;

    RulerStrip(const TimelineGeometry& geometryRef, const DisplayedTimeline& displayedTimelineRef);

    std::function<void(int tick)> onSeek;
    std::function<void(const juce::MouseEvent&, const juce::MouseWheelDetails&)> onWheelZoom;
    std::function<void(const juce::MouseEvent&, int deltaY)> onDragZoom;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    bool isDragging = false;
    int dragStartY = 0;
    int lastDragY = 0;
};
