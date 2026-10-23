#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace DragValueTipPainter
{
// The part of owner that its enclosing viewport shows.
juce::Rectangle<int> visibleArea(const juce::Component& owner);
// Draws the tip next to mouse, inside area, in owner's tooltip colours.
void draw(juce::Graphics& g, const juce::Component& owner, juce::Point<int> mouse, juce::Rectangle<int> area,
          const juce::String& position, const juce::String& value);
} // namespace DragValueTipPainter
