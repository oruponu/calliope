#include "ui/pianoroll/strips/RangeBandPainter.h"

void RangeBandPainter::draw(juce::Graphics& g, const RangeSelectGesture& gesture, int height, juce::Colour fillColour,
                            juce::Colour borderColour)
{
    const int lo = gesture.lo();
    const int hi = gesture.hi();
    if (hi <= lo)
        return;

    juce::Rectangle<float> band(static_cast<float>(lo), 0.0f, static_cast<float>(hi - lo), static_cast<float>(height));
    g.setColour(fillColour);
    g.fillRect(band);
    g.setColour(borderColour);
    g.drawRect(band, 1.0f);
}
