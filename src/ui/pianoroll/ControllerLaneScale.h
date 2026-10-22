#pragma once

#include <algorithm>
#include <cstdlib>
#include <utility>
#include <vector>

namespace ControllerLaneScale
{
inline constexpr int pitchBendCentre = 8192;
inline constexpr int pitchBendMax = 16383;
// One pixel is about 160 steps, so the mouse could not land on the centre without this.
inline constexpr int pitchBendSnapPixels = 3;

// offsetFromCentre is in pixels, positive upwards.
inline int pitchBendAt(int offsetFromCentre, int halfHeight)
{
    if (std::abs(offsetFromCentre) <= pitchBendSnapPixels || halfHeight <= 0)
        return pitchBendCentre;
    return std::clamp(pitchBendCentre + offsetFromCentre * pitchBendCentre / halfHeight, 0, pitchBendMax);
}

// A point's y can read back as a nearby value, so a sideways drag would otherwise change its value.
template <class ValueAtY> int draggedValue(int startValue, int startY, int deltaY, ValueAtY valueAtY)
{
    return deltaY == 0 ? startValue : valueAtY(startY + deltaY);
}

// Within the slop an off-grid point keeps its tick, so a vertical drag does not snap it past its neighbours.
inline constexpr int dragSlopPixels = 2;

template <class SnappedTickAtX> int draggedTick(int startTick, int startX, int deltaX, SnappedTickAtX snappedTickAtX)
{
    return std::abs(deltaX) <= dragSlopPixels ? startTick : snappedTickAtX(startX + deltaX);
}

// offsets are (dx, dy) from the mouse to each point in drawing order; a tie goes to the point drawn last.
inline int nearestWithin(const std::vector<std::pair<float, float>>& offsets, float radius)
{
    int nearest = -1;
    float nearestDistance = radius * radius;
    for (int i = 0; i < static_cast<int>(offsets.size()); ++i)
    {
        const auto [dx, dy] = offsets[static_cast<size_t>(i)];
        if (const float distance = dx * dx + dy * dy; distance <= nearestDistance)
        {
            nearest = i;
            nearestDistance = distance;
        }
    }
    return nearest;
}
} // namespace ControllerLaneScale
