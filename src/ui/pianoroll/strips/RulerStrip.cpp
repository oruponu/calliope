#include "ui/pianoroll/strips/RulerStrip.h"
#include "ui/pianoroll/GridLineColours.h"
#include "ui/theme/Theme.h"

RulerStrip::RulerStrip(const TimelineGeometry& geometryRef, const DisplayedTimeline& displayedTimelineRef)
    : TimelineStrip(geometryRef, displayedTimelineRef, {})
{
}

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

    const auto& timeline = displayedTimeline.get();
    // A bar number starts right of its line, so bar lines just left of the clip still get their numbers drawn.
    const int labelReach = 30;
    for (const auto& line :
         timelineGrid().lines(geometry.xToTick(visibleLeft - labelReach), geometry.xToTick(visibleRight + 1)))
    {
        const int x = geometry.tickToX(line.tick);
        if (x < visibleLeft - labelReach || x > visibleRight)
            continue;

        g.setColour(gridLineColour(line.kind));
        if (line.kind == GridLineKind::Sub)
        {
            const int tickH = getHeight() / 3;
            g.drawVerticalLine(x, static_cast<float>(getHeight() - tickH), static_cast<float>(getHeight()));
            continue;
        }

        g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));
        if (line.kind == GridLineKind::Bar)
        {
            g.setColour(text::t2);
            g.setFont(font::sans(font::sizeSM));
            g.drawText(juce::String(timeline.tickToBarBeatTick(line.tick).bar), x + 4, 2, 30, getHeight() - 4,
                       juce::Justification::centredLeft);
        }
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

    int tick = std::max(0, snapGrid({}).round(geometry.xToTick(e.x)));
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
