#pragma once

#include "ui/pianoroll/strips/TimelineStrip.h"

class KeySignatureStrip : public TimelineStrip
{
public:
    static constexpr int height = 24;

    explicit KeySignatureStrip(const TimelineGeometry& geometryRef);

    void paint(juce::Graphics& g) override;

private:
    void timelineMetadataChanged() override { repaint(); }
};
