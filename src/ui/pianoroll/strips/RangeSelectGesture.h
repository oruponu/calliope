#pragma once

#include "ui/pianoroll/TimelineGeometry.h"
#include <algorithm>
#include <set>

struct RangeSelectGesture
{
    int startX = 0;
    int currentX = 0;
    std::set<int> base;

    int lo() const { return std::min(startX, currentX); }
    int hi() const { return std::max(startX, currentX); }

    template <class InRange>
    std::set<int> selectionFor(int count, const TimelineGeometry& geometry, InRange inRange) const
    {
        const int tickLo = geometry.xToTick(lo());
        const int tickHi = geometry.xToTick(hi());
        std::set<int> result = base;
        for (int i = 0; i < count; ++i)
            if (inRange(i, tickLo, tickHi))
                result.insert(i);
        return result;
    }
};
