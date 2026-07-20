#pragma once

#include "ui/pianoroll/strips/TimelineStrip.h"

class ChordStrip : public TimelineStrip
{
public:
    static constexpr int height = 24;

    explicit ChordStrip(const TimelineGeometry& geometryRef);

    void paint(juce::Graphics& g) override;

private:
    void timelineMetadataChanged() override { repaint(); }
};
