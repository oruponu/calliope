#include "ui/pianoroll/strips/LoopStrip.h"
#include "ui/theme/Theme.h"

LoopStrip::LoopStrip(const TimelineGeometry& geometryRef) : TimelineStrip(geometryRef, {}) {}

void LoopStrip::paint(juce::Graphics& g)
{
    using namespace calliope::theme;
    if (sequence == nullptr)
        return;

    g.setColour(surface::bg2);
    g.fillRect(viewLeftX, 0, getWidth() - viewLeftX, getHeight());

    g.saveState();
    g.reduceClipRegion(viewLeftX + labelWidth(), 0, getWidth(), getHeight());

    if (loopEndTick > loopStartTick)
    {
        float lx1 = static_cast<float>(geometry.tickToX(loopStartTick));
        float lx2 = static_cast<float>(geometry.tickToX(loopEndTick));

        auto fillColour = loopEnabled ? accent::soft : surface::hover;
        auto edgeColour = loopEnabled ? accent::base : text::t4;

        g.setColour(fillColour);
        g.fillRoundedRectangle(lx1, 2.0f, lx2 - lx1 + 1.0f, static_cast<float>(getHeight() - 4), 2.0f);

        g.setColour(edgeColour);
        g.fillRect(lx1, 2.0f, 2.0f, static_cast<float>(getHeight() - 4));
        g.fillRect(lx2 - 1.0f, 2.0f, 2.0f, static_cast<float>(getHeight() - 4));
    }

    float phX = playheadX();
    if (phX >= static_cast<float>(viewLeftX + labelWidth()) && phX <= static_cast<float>(getWidth()))
    {
        g.setColour(text::t1);
        g.drawLine(phX, 0.0f, phX, static_cast<float>(getHeight()), 1.0f);
    }

    g.restoreState();

    g.setColour(border::normal);
    g.drawHorizontalLine(getHeight() - 1, static_cast<float>(viewLeftX), static_cast<float>(getWidth()));
}

void LoopStrip::mouseDown(const juce::MouseEvent& e)
{
    if (sequence == nullptr || sequence->getNumTracks() == 0)
        return;
    if (e.x < viewLeftX + labelWidth())
        return;

    int tick = geometry.roundTickToGrid(geometry.xToTick(e.x));
    dragStartTick = std::max(0, tick);
    isDragging = true;
}

void LoopStrip::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging || sequence == nullptr)
        return;

    int tick = std::max(0, geometry.roundTickToGrid(geometry.xToTick(e.x)));
    int start = std::min(dragStartTick, tick);
    int end = std::max(dragStartTick, tick);
    if (end > start)
    {
        loopStartTick = start;
        loopEndTick = end;
        repaint();
        if (onLoopRegionChanged)
            onLoopRegionChanged(loopStartTick, loopEndTick);
    }
}

void LoopStrip::mouseUp(const juce::MouseEvent&)
{
    isDragging = false;
}
