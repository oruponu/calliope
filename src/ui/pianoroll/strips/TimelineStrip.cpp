#include "ui/pianoroll/strips/TimelineStrip.h"
#include "ui/theme/Theme.h"

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

void TimelineStrip::drawLoopOverlay(juce::Graphics& g, int top, int height, float fillAlpha)
{
    using namespace calliope::theme;
    if (loopEndTick <= loopStartTick)
        return;

    float lx1 = static_cast<float>(geometry.tickToX(loopStartTick));
    float lx2 = static_cast<float>(geometry.tickToX(loopEndTick));
    float fTop = static_cast<float>(top);
    float fBottom = static_cast<float>(top + height);

    g.setColour(loopEnabled ? accent::base.withAlpha(fillAlpha) : surface::hover);
    g.fillRect(lx1, fTop, lx2 - lx1, static_cast<float>(height));

    g.setColour(loopEnabled ? accent::base.withAlpha(0.7f) : text::t4);
    g.drawVerticalLine(static_cast<int>(lx1), fTop, fBottom);
    g.drawVerticalLine(static_cast<int>(lx2), fTop, fBottom);
}
