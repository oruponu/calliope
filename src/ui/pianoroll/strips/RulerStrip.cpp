#include "ui/pianoroll/strips/RulerStrip.h"
#include "ui/theme/Theme.h"

RulerStrip::RulerStrip(const TimelineGeometry& geometryRef) : TimelineStrip(geometryRef, {}) {}

void RulerStrip::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (sequence == nullptr)
        return;

    auto clip = g.getClipBounds();
    int visibleLeft = clip.getX();
    int visibleRight = clip.getRight();

    g.setColour(border::normal);
    g.fillRect(viewLeftX, 0, getWidth() - viewLeftX, getHeight());

    g.saveState();
    g.reduceClipRegion(viewLeftX + labelWidth(), 0, getWidth(), getHeight());

    int ppq = sequence->getTicksPerQuarterNote();
    int quantizeGrid = geometry.gridTicks();
    int totalTicks = geometry.xToTick(getWidth());
    int tick = 0;
    int barNumber = 1;

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
            barNumber++;
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
                if (x < visibleLeft - 30)
                    continue;

                if (sub == 0)
                {
                    bool isBar = (beat == 0);
                    g.setColour(isBar ? border::strong : border::normal);
                    g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));

                    if (isBar)
                    {
                        g.setColour(text::t2);
                        g.setFont(font::sans(font::sizeSM));
                        g.drawText(juce::String(barNumber), x + 4, 2, 30, getHeight() - 4,
                                   juce::Justification::centredLeft);
                    }
                }
                else
                {
                    g.setColour(border::soft);
                    int tickH = getHeight() / 3;
                    g.drawVerticalLine(x, static_cast<float>(getHeight() - tickH), static_cast<float>(getHeight()));
                }
            }
        }

        tick = barEndTick;
        barNumber++;
    }

    float phX = playheadX();
    if (phX >= static_cast<float>(visibleLeft) - 1.0f && phX <= static_cast<float>(visibleRight) + 1.0f)
    {
        g.setColour(text::t1);
        g.drawLine(phX, 0.0f, phX, static_cast<float>(getHeight()), 1.0f);
    }

    drawLoopOverlay(g, 0, getHeight(), 0.35f);

    g.restoreState();

    g.setColour(border::normal);
    g.drawVerticalLine(viewLeftX + labelWidth() - 1, 0.0f, static_cast<float>(getHeight()));

    g.setColour(border::strong);
    g.drawHorizontalLine(getHeight() - 1, static_cast<float>(viewLeftX), static_cast<float>(getWidth()));
}

void RulerStrip::mouseDown(const juce::MouseEvent& e)
{
    if (sequence == nullptr || sequence->getNumTracks() == 0)
        return;
    if (e.x < viewLeftX + labelWidth())
        return;

    int tick = std::max(0, geometry.roundTickToGrid(geometry.xToTick(e.x)));
    if (onSeek)
        onSeek(tick);
    isDragging = true;
    dragStartY = e.y;
    lastDragY = e.y;
}

void RulerStrip::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging)
        return;
    if (std::abs(e.y - dragStartY) < 5)
    {
        lastDragY = e.y;
        return;
    }
    int deltaY = e.y - lastDragY;
    lastDragY = e.y;
    if (deltaY != 0 && onDragZoom)
        onDragZoom(e, deltaY);
}

void RulerStrip::mouseUp(const juce::MouseEvent&)
{
    isDragging = false;
}

void RulerStrip::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (onWheelZoom)
    {
        onWheelZoom(e, wheel);
        return;
    }
    TimelineStrip::mouseWheelMove(e, wheel);
}
