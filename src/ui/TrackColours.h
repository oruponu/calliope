#pragma once

#include <juce_graphics/juce_graphics.h>

namespace TrackColours
{
inline const juce::Colour colours[] = {
    {100, 160, 255}, // Blue
    {100, 210, 140}, // Green
    {255, 170, 80},  // Orange
    {180, 130, 255}, // Purple
    {255, 120, 160}, // Pink
    {80, 200, 220},  // Cyan
    {220, 200, 100}, // Yellow
    {200, 140, 120}, // Brown
};
inline constexpr int numColours = 8;

inline juce::Colour getColour(int index)
{
    return colours[index % numColours];
}
} // namespace TrackColours
