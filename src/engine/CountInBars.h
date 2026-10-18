#pragma once

#include <algorithm>

struct CountInBars
{
    static constexpr int min = 1;
    static constexpr int max = 4;
    static constexpr int defaultBars = 1;

    static int clamped(int bars) { return std::clamp(bars, min, max); }
};
