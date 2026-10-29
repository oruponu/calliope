#pragma once

#include "model/MidiSequence.h"
#include "model/grid/TimelineGrid.h"
#include "ui/pianoroll/DisplayedTimeline.h"
#include "ui/pianoroll/TimelineGeometry.h"
#include "ui/pianoroll/strips/RangeSelectGesture.h"
#include <juce_gui_basics/juce_gui_basics.h>

class TimelineStrip : public juce::Component, public MidiSequence::Listener
{
public:
    TimelineStrip(const TimelineGeometry& geometryRef, const DisplayedTimeline& displayedTimelineRef,
                  const juce::String& labelText);
    ~TimelineStrip() override;

    virtual void setSequence(MidiSequence* seq);
    void setPlayheadTick(double tick);
    void setLoopRegion(bool enabled, int startTick, int endTick);
    void setViewLeftX(int x);

protected:
    virtual void cancelDrag() {}

    int labelWidth() const { return geometry.timelineStartX(); }
    float playheadX() const;
    void drawLabelColumn(juce::Graphics& g);
    void drawLoopOverlay(juce::Graphics& g, int top, int height, float fillAlpha);
    void drawTrackGridLines(juce::Graphics& g, int visibleLeft, int visibleRight, float top, float bottom);
    // Needs a sequence, like displayedTimeline.get().
    TimelineGrid timelineGrid() const { return {displayedTimeline.get(), geometry.gridDivision()}; }
    // The grid that mouse edits land on: timelineGrid() while snapping, otherwise every tick. Needs a sequence too.
    TimelineGrid snapGrid(const juce::ModifierKeys& mods) const;

    const TimelineGeometry& geometry;
    const DisplayedTimeline& displayedTimeline;
    MidiSequence* sequence = nullptr;
    juce::String label;
    double playheadTick = 0.0;
    bool loopEnabled = false;
    int loopStartTick = 0;
    int loopEndTick = 0;
    int viewLeftX = 0;

private:
    void notesChanged(int trackIndex) override;
    void tracksChanged() override;
    void tempoChanged() override;
    void timelineMetadataChanged() override;
    void sequenceReset() override;
    void modelChanged();
};
