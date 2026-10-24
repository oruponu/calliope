#pragma once

#include "ui/pianoroll/VelocityBar.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

// Where the mouse grabbed a bar's top and the velocity the note had then.
struct VelocityTopGrab
{
    int pressY = 0;
    int velocity = 0;
};

namespace VelocityBarGrab
{
// The index of the bar whose top the mouse is on, or -1. A bar is hit from margin left of it to margin right of it.
// Of tops equally near, the later bar wins.
inline int hit(const std::vector<VelocityBar>& bars, int mouseX, int mouseY, int width, int margin, int tolerance)
{
    int best = -1;
    int bestDistance = tolerance + 1;
    for (int i = 0; i < static_cast<int>(bars.size()); ++i)
    {
        const auto& bar = bars[static_cast<size_t>(i)];
        const int distance = std::abs(bar.top - mouseY);
        if (mouseX >= bar.x - margin && mouseX < bar.x + width + margin && distance <= bestDistance)
        {
            best = i;
            bestDistance = distance;
        }
    }
    return best;
}

inline int draggedValue(const VelocityTopGrab& grab, int mouseY, int areaHeight)
{
    const double moved = static_cast<double>(grab.pressY - mouseY) * 127.0 / areaHeight;
    return std::clamp(grab.velocity + static_cast<int>(std::lround(moved)), 0, 127);
}
} // namespace VelocityBarGrab
