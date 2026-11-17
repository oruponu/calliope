#include "edit/time/TimeSignatureBars.h"

bool timeSignaturesStartBars(const std::vector<TimeSignatureChange>& changes, int ppq)
{
    for (size_t i = 1; i < changes.size(); ++i)
    {
        const auto& previous = changes[i - 1];
        if (previous.denominator <= 0)
            return false;
        const long long barLength = 4LL * ppq / previous.denominator * previous.numerator;
        if (barLength <= 0)
            return false;
        if ((static_cast<long long>(changes[i].tick) - previous.tick) % barLength != 0)
            return false;
    }
    return true;
}
