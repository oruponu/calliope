#include "ui/pianoroll/strips/TimelineStrip.h"
#include "ui/theme/Theme.h"
#include <algorithm>

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
    using namespace calliope::theme;
    if (sequence == nullptr)
        return;

    int ppq = sequence->getTicksPerQuarterNote();
    int quantizeGrid = geometry.gridTicks();
    int totalTicks = geometry.xToTick(getWidth());
    int tick = 0;

    while (tick < totalTicks)
    {
        auto ts = sequence->getTimeSignatureAt(tick);
        int ticksPerBeat = ppq * 4 / ts.denominator;
        int beatsInBar = ts.numerator;
        int barEndTick = tick + beatsInBar * ticksPerBeat;

        if (geometry.tickToX(tick) > visibleRight)
            break;

        if (geometry.tickToX(barEndTick) < visibleLeft)
        {
            tick = barEndTick;
            continue;
        }

        int subdivisionsPerBeat = std::max(1, ticksPerBeat / quantizeGrid);

        for (int beat = 0; beat < beatsInBar && tick + beat * ticksPerBeat <= totalTicks; ++beat)
        {
            int beatTick = tick + beat * ticksPerBeat;

            for (int sub = 0; sub < subdivisionsPerBeat; ++sub)
            {
                int subTick = beatTick + sub * quantizeGrid;
                int x = geometry.tickToX(subTick);
                if (x > visibleRight)
                    break;
                if (x < visibleLeft)
                    continue;

                if (sub == 0)
                {
                    bool isBar = (beat == 0);
                    g.setColour(isBar ? border::strong : border::normal);
                }
                else
                {
                    g.setColour(border::soft);
                }
                g.drawVerticalLine(x, top, bottom);
            }
        }

        tick = barEndTick;
    }
}
