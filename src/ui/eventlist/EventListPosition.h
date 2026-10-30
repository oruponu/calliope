#pragma once

#include "model/timeline/BarBeatTick.h"
#include "model/timeline/TimelineMap.h"
#include "ui/eventlist/PositionField.h"

namespace EventListPosition
{
// The bar is kept at or after the first, and the beat and tick inside the bar's time signature.
int toTick(const BarBeatTick& position, const TimelineMap& timeline);
// One step of the field, carried into the fields above it, after the position is kept inside its bar. A bar step keeps
// the beat and tick inside the new bar. A step that would go before the first tick leaves the position where it is.
BarBeatTick stepped(const BarBeatTick& position, PositionField field, int direction, const TimelineMap& timeline);
} // namespace EventListPosition
