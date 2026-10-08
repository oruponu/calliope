#pragma once

#include <algorithm>

struct MetronomeVolume
{
    static constexpr int min = 0;
    static constexpr int max = 100;
    static constexpr int step = 5;
    static constexpr int defaultPercent = 70;

    static int nudged(int percent, int direction) { return clamped(percent + direction * step); }

    static int clamped(int percent) { return std::clamp(percent, min, max); }

    // Squared so that each step changes loudness by a similar amount; linear gain leaves the upper half almost flat.
    static float gainFor(int percent)
    {
        const float level = static_cast<float>(percent) / 100.0f;
        return level * level;
    }
};
