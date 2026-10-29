#pragma once

#include "model/grid/GridLineKind.h"
#include "ui/theme/Theme.h"

inline juce::Colour gridLineColour(GridLineKind kind)
{
    using namespace calliope::theme;
    switch (kind)
    {
    case GridLineKind::Bar:
        return border::strong;
    case GridLineKind::Beat:
        return border::normal;
    case GridLineKind::Sub:
        break;
    }
    return border::soft;
}
