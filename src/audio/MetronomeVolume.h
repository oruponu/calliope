#pragma once

#include <algorithm>

struct MetronomeVolume
{
    static constexpr int min = 0;
    static constexpr int max = 100;
    static constexpr int defaultPercent = 70;

    static int clamped(int percent) { return std::clamp(percent, min, max); }

    // Squared so that equal slider moves change loudness by similar amounts;
    // linear gain leaves the upper half almost flat.
    static float gainFor(int percent)
    {
        const float level = static_cast<float>(percent) / 100.0f;
        return level * level;
    }
};
