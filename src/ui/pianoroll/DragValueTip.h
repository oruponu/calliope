#pragma once

#include "model/BarBeatTick.h"
#include <algorithm>
#include <string>

// The tip shown next to the mouse while a point is dragged, with where the point lands and its value.
namespace DragValueTip
{
inline std::string formatPosition(const BarBeatTick& position)
{
    return std::to_string(position.bar) + "." + std::to_string(position.beat) + "." + std::to_string(position.tick);
}

// The start of a tip of size placed gap past the mouse, or before it when that leaves the area; kept inside the area as
// far as it fits.
inline int placeAfter(int mouse, int size, int gap, int areaStart, int areaEnd)
{
    int start = mouse + gap;
    if (start + size > areaEnd)
        start = mouse - gap - size;
    return std::max(areaStart, std::min(start, areaEnd - size));
}

// As placeAfter, preferring the side before the mouse.
inline int placeBefore(int mouse, int size, int gap, int areaStart, int areaEnd)
{
    int start = mouse - gap - size;
    if (start < areaStart)
        start = mouse + gap;
    return std::max(areaStart, std::min(start, areaEnd - size));
}
} // namespace DragValueTip
