#include "ui/pianoroll/strips/TimelineStrip.h"

TimelineStrip::TimelineStrip(const TimelineGeometry& geometryRef, const juce::String& labelText)
    : geometry(geometryRef), label(labelText)
{
    setWantsKeyboardFocus(false);
}

TimelineStrip::~TimelineStrip()
{
    if (sequence != nullptr)
        sequence->removeListener(this);
}

void TimelineStrip::setSequence(MidiSequence* seq)
{
    if (sequence != nullptr)
        sequence->removeListener(this);
    sequence = seq;
    if (sequence != nullptr)
        sequence->addListener(this);
    repaint();
}

void TimelineStrip::setPlayheadTick(double tick)
{
    const int oldX = static_cast<int>(playheadX());
    playheadTick = tick;
    const int newX = static_cast<int>(playheadX());
    constexpr int margin = 2;
    repaint(oldX - margin, 0, margin * 2 + 2, getHeight());
    repaint(newX - margin, 0, margin * 2 + 2, getHeight());
}

void TimelineStrip::setLoopRegion(bool enabled, int startTick, int endTick)
{
    loopEnabled = enabled;
    loopStartTick = startTick;
    loopEndTick = endTick;
    repaint();
}

void TimelineStrip::setViewLeftX(int x)
{
    if (viewLeftX == x)
        return;
    repaint(viewLeftX, 0, labelWidth(), getHeight());
    viewLeftX = x;
    repaint(viewLeftX, 0, labelWidth(), getHeight());
}

float TimelineStrip::playheadX() const
{
    if (sequence == nullptr)
        return static_cast<float>(geometry.timelineStartX());
    return static_cast<float>(geometry.timelineStartX() +
                              playheadTick / sequence->getTicksPerQuarterNote() * geometry.getBeatWidth());
}
