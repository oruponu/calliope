#pragma once

#include "model/timeline/BarBeatTick.h"
#include "model/timeline/TimelineMap.h"

namespace EventListPosition
{
// The bar is kept at or after the first, and the beat and tick inside the bar's time signature.
int toTick(const BarBeatTick& position, const TimelineMap& timeline);
} // namespace EventListPosition
