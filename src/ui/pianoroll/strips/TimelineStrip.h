#pragma once

#include "model/MidiSequence.h"
#include "ui/pianoroll/TimelineGeometry.h"
#include <juce_gui_basics/juce_gui_basics.h>

class TimelineStrip : public juce::Component, public MidiSequence::Listener
{
public:
    TimelineStrip(const TimelineGeometry& geometryRef, const juce::String& labelText);
    ~TimelineStrip() override;

    virtual void setSequence(MidiSequence* seq);
    void setPlayheadTick(double tick);
    void setLoopRegion(bool enabled, int startTick, int endTick);
    void setViewLeftX(int x);

protected:
    int labelWidth() const { return geometry.timelineStartX(); }
    float playheadX() const;

    const TimelineGeometry& geometry;
    MidiSequence* sequence = nullptr;
    juce::String label;
    double playheadTick = 0.0;
    bool loopEnabled = false;
    int loopStartTick = 0;
    int loopEndTick = 0;
    int viewLeftX = 0;
};
