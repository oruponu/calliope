#include "ui/pianoroll/DragValueTipPainter.h"
#include "ui/pianoroll/DragValueTip.h"
#include "ui/theme/Theme.h"
#include <algorithm>

juce::Rectangle<int> DragValueTipPainter::visibleArea(const juce::Component& owner)
{
    auto area = owner.getLocalBounds();
    if (const auto* viewport = owner.findParentComponentOfClass<juce::Viewport>())
        area = area.getIntersection(owner.getLocalArea(viewport, viewport->getLocalBounds()));
    return area;
}

void DragValueTipPainter::draw(juce::Graphics& g, const juce::Component& owner, juce::Point<int> mouse,
                               juce::Rectangle<int> area, const juce::String& position, const juce::String& value)
{
    using namespace calliope::theme;
    constexpr int padding = 4;
    constexpr int lineHeight = 14;
    constexpr int gap = 12;

    const auto tipFont = font::mono(font::sizeSM);
    const int width = std::max(juce::GlyphArrangement::getStringWidthInt(tipFont, position),
                               juce::GlyphArrangement::getStringWidthInt(tipFont, value)) +
                      padding * 2;
    const int height = lineHeight * 2 + padding * 2;
    const juce::Rectangle<int> tip{DragValueTip::placeAfter(mouse.x, width, gap, area.getX(), area.getRight()),
                                   DragValueTip::placeBefore(mouse.y, height, gap, area.getY(), area.getBottom()),
                                   width, height};

    g.setColour(owner.findColour(juce::TooltipWindow::backgroundColourId));
    g.fillRect(tip);
    g.setColour(owner.findColour(juce::TooltipWindow::outlineColourId));
    g.drawRect(tip);
    g.setColour(owner.findColour(juce::TooltipWindow::textColourId));
    g.setFont(tipFont);
    auto text = tip.reduced(padding);
    g.drawText(position, text.removeFromTop(lineHeight), juce::Justification::centredLeft, false);
    g.drawText(value, text, juce::Justification::centredLeft, false);
}
