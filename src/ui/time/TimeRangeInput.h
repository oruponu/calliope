#pragma once

#include "model/timeline/BarBeatTick.h"
#include "model/timeline/TimelineMap.h"
#include <optional>
#include <string_view>

namespace TimeRangeInput
{
inline constexpr int maxBar = 9999;

// Whole numbers only, surrounding spaces ignored; bar from 1 to maxBar, beat from 1, tick from 0, each within int.
std::optional<BarBeatTick> parse(std::string_view bar, std::string_view beat, std::string_view tick);
// The position of the tick, kept inside the bars that can be typed.
BarBeatTick initialStart(int tick, const TimelineMap& timeline);
// The position with its bar, beat and tick kept inside the bar's time signature, as the event list keeps it.
BarBeatTick keptInside(const BarBeatTick& position, const TimelineMap& timeline);
// The same beat and tick in the next bar, kept inside that bar's time signature.
BarBeatTick initialEnd(const BarBeatTick& start, const TimelineMap& timeline);
} // namespace TimeRangeInput
