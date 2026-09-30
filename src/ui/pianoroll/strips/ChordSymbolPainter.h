#pragma once

#include "notation/ChordSymbolParts.h"
#include <juce_graphics/juce_graphics.h>

namespace ChordSymbolPainter
{
// Draws the type as a superscript, clipped to area.
void draw(juce::Graphics& g, const ChordSymbolParts& parts, juce::Rectangle<int> area);
} // namespace ChordSymbolPainter
