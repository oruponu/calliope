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

int beatsIn(int bar, const TimelineMap& timeline)
{
    return timeline.getTimeSignatureAt(timeline.barStartToTick(bar)).numerator;
}
} // namespace

int EventListPosition::toTick(const BarBeatTick& position, const TimelineMap& timeline)
{
    const auto inside = clamped(position, timeline);
    return timeline.barBeatTickToTick(inside.bar, inside.beat, inside.tick);
}

BarBeatTick EventListPosition::stepped(const BarBeatTick& position, PositionField field, int direction,
                                       const TimelineMap& timeline)
{
    const auto from = clamped(position, timeline);
    switch (field)
    {
    case PositionField::Bar:
        if (from.bar + direction < 1)
            return from;
        return clamped({from.bar + direction, from.beat, from.tick}, timeline);
    case PositionField::Beat:
        if (direction > 0)
            return from.beat < beatsIn(from.bar, timeline) ? clamped({from.bar, from.beat + 1, from.tick}, timeline)
                                                           : clamped({from.bar + 1, 1, from.tick}, timeline);
        if (from.beat > 1)
            return {from.bar, from.beat - 1, from.tick};
        if (from.bar == 1)
            return from;
        return clamped({from.bar - 1, beatsIn(from.bar - 1, timeline), from.tick}, timeline);
    case PositionField::Tick:
        return timeline.tickToBarBeatTick(std::max(0, toTick(from, timeline) + direction));
    }
    return from;
}
