#pragma once

#include "Theme.h"
#include <juce_graphics/juce_graphics.h>

namespace TrackColours
{
inline juce::Colour getColour(int index)
{
    const auto& palette = calliope::theme::track::palette();
    return palette[index % palette.size()];
}
} // namespace TrackColours
