#pragma once

#include <algorithm>

struct GridDivision
{
    int denominator = 16;

    bool operator==(const GridDivision&) const = default;

    // The step is stepNumerator / stepDenominator ticks, kept as a fraction so that lines are placed without drift.
    long long stepNumerator(int ppq) const { return 4LL * ppq; }
    long long stepDenominator() const { return denominator; }

    int stepTicks(int ppq) const
    {
        const long long rounded = (stepNumerator(ppq) + stepDenominator() / 2) / stepDenominator();
        return static_cast<int>(std::max(1LL, rounded));
    }
};
