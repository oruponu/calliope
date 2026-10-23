#pragma once

#include "ui/pianoroll/strips/RangeSelectGesture.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace RangeBandPainter
{
void draw(juce::Graphics& g, const RangeSelectGesture& gesture, int height, juce::Colour fillColour,
          juce::Colour borderColour);
} // namespace RangeBandPainter
