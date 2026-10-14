#pragma once

#include "model/TimelineGrid.h"
#include "model/TimelineMap.h"

namespace gridtest
{
inline TimelineMap threeFourTimeline()
{
    TimelineMap timeline;
    timeline.setTimeSignatureChanges({{0, 3, 4}});
    return timeline;
}

inline const TimelineMap fourFour;
inline const TimelineMap threeFour = threeFourTimeline();
inline const TimelineGrid fourFourSixteenths(fourFour, {16});
inline const TimelineGrid fourFourQuarters(fourFour, {4});
inline const TimelineGrid threeFourQuarterTriplets(threeFour, {4, true});
} // namespace gridtest
