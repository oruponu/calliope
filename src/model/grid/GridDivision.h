#pragma once

#include <algorithm>

struct GridDivision
{
    int denominator = 16;
    bool triplet = false;

    bool operator==(const GridDivision&) const = default;

    // The step is stepNumerator / stepDenominator ticks, kept as a fraction so that triplet lines do not drift.
    long long stepNumerator(int ppq) const { return 4LL * ppq * (triplet ? 2 : 1); }
    long long stepDenominator() const { return static_cast<long long>(denominator) * (triplet ? 3 : 1); }

    int stepTicks(int ppq) const
    {
        const long long rounded = (stepNumerator(ppq) + stepDenominator() / 2) / stepDenominator();
        return static_cast<int>(std::max(1LL, rounded));
    }
};
