#include "ui/eventlist/EventListPosition.h"
#include <algorithm>

namespace
{
// Keeps bar starts inside int unless the bars are unusually long.
constexpr int kMaxBar = 9999;

BarBeatTick clamped(const BarBeatTick& position, const TimelineMap& timeline)
{
    const int bar = std::clamp(position.bar, 1, kMaxBar);
    const auto signature = timeline.getTimeSignatureAt(timeline.barStartToTick(bar));
    const int ticksPerBeat = timeline.getTicksPerQuarterNote() * 4 / signature.denominator;
    return {bar, std::clamp(position.beat, 1, signature.numerator), std::clamp(position.tick, 0, ticksPerBeat - 1)};
}
} // namespace

int EventListPosition::toTick(const BarBeatTick& position, const TimelineMap& timeline)
{
    const auto inside = clamped(position, timeline);
    return timeline.barBeatTickToTick(inside.bar, inside.beat, inside.tick);
}
