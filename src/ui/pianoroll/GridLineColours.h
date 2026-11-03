#pragma once

#include "model/grid/GridLineKind.h"
#include "ui/theme/Theme.h"

inline juce::Colour gridLineColour(GridLineKind kind)
{
    switch (kind)
    {
    case GridLineKind::Bar:
        return juce::Colours::white.withAlpha(0.15f);
    case GridLineKind::Beat:
        return juce::Colours::white.withAlpha(0.1f);
    case GridLineKind::Sub:
        break;
    }
    return juce::Colours::white.withAlpha(0.05f);
}
