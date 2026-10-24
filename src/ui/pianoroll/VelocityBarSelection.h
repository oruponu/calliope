#pragma once

#include "ui/pianoroll/VelocityBar.h"
#include <vector>

namespace VelocityBarSelection
{
// The indices of the bars that overlap the range from lo to hi, with each bar covering x to x + width - 1.
inline std::vector<int> overlapping(const std::vector<VelocityBar>& bars, int lo, int hi, int width)
{
    std::vector<int> result;
    for (int i = 0; i < static_cast<int>(bars.size()); ++i)
    {
        const auto& bar = bars[static_cast<size_t>(i)];
        if (bar.x <= hi && bar.x + width > lo)
            result.push_back(i);
    }
    return result;
}
} // namespace VelocityBarSelection
