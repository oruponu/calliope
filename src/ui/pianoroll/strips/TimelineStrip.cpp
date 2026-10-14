#include "ui/pianoroll/strips/TimelineStrip.h"
#include "ui/pianoroll/GridLineColours.h"
#include "ui/theme/Theme.h"

TimelineStrip::TimelineStrip(const TimelineGeometry& geometryRef, const DisplayedTimeline& displayedTimelineRef,
                             const juce::String& labelText)
    : geometry(geometryRef), displayedTimeline(displayedTimelineRef), label(labelText)
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
    return static_cast<float>(geometry.timelineStartX() + playheadTick /
                                                              sequence->getTimeline().getTicksPerQuarterNote() *
                                                              geometry.getBeatWidth());
}

void TimelineStrip::drawLabelColumn(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (label.isNotEmpty())
    {
        g.setColour(text::t3);
        g.setFont(font::sans(font::sizeXS));
        g.drawText(label, viewLeftX + 4, 0, labelWidth() - 8, getHeight(), juce::Justification::centredLeft);
    }
    g.setColour(border::normal);
    g.drawVerticalLine(viewLeftX + labelWidth() - 1, 0.0f, static_cast<float>(getHeight()));
    g.setColour(border::strong);
    g.drawHorizontalLine(getHeight() - 1, static_cast<float>(viewLeftX), static_cast<float>(getWidth()));
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

void TimelineStrip::drawTrackGridLines(juce::Graphics& g, int visibleLeft, int visibleRight, float top, float bottom)
{
    if (sequence == nullptr)
        return;

    for (const auto& line : timelineGrid().lines(geometry.xToTick(visibleLeft - 1), geometry.xToTick(visibleRight + 1)))
    {
        const int x = geometry.tickToX(line.tick);
        if (x < visibleLeft || x > visibleRight)
            continue;
        g.setColour(gridLineColour(line.kind));
        g.drawVerticalLine(x, top, bottom);
    }
}

void TimelineStrip::drawRangeBand(juce::Graphics& g, const RangeSelectGesture& gesture, juce::Colour fillColour,
                                  juce::Colour borderColour)
{
    const int lo = gesture.lo();
    const int hi = gesture.hi();
    if (hi <= lo)
        return;

    juce::Rectangle<float> band(static_cast<float>(lo), 0.0f, static_cast<float>(hi - lo),
                                static_cast<float>(getHeight()));
    g.setColour(fillColour);
    g.fillRect(band);
    g.setColour(borderColour);
    g.drawRect(band, 1.0f);
}

void TimelineStrip::notesChanged(int)
{
    modelChanged();
}

void TimelineStrip::tracksChanged()
{
    modelChanged();
}

void TimelineStrip::tempoChanged()
{
    modelChanged();
}

void TimelineStrip::timelineMetadataChanged()
{
    modelChanged();
}

void TimelineStrip::sequenceReset()
{
    modelChanged();
}

void TimelineStrip::modelChanged()
{
    cancelDrag();
    repaint();
}
