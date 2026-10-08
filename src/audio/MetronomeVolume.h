#pragma once

struct MetronomeVolume
{
    static constexpr int defaultPercent = 70;

    // Squared so that each step changes loudness by a similar amount; linear gain leaves the upper half almost flat.
    static float gainFor(int percent)
    {
        const float level = static_cast<float>(percent) / 100.0f;
        return level * level;
    }
};
