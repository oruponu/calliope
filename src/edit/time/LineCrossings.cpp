#include "edit/time/LineCrossings.h"
#include "model/controller/ControllerKind.h"
#include "model/controller/ControllerRamps.h"
#include <algorithm>
#include <iterator>
#include <tuple>

namespace
{
bool kindLess(const ControllerKind& a, const ControllerKind& b)
{
    return std::tie(a.type, a.number) < std::tie(b.type, b.number);
}
} // namespace

std::vector<LineCrossing> lineCrossings(const std::vector<MidiEvent>& events, int tick, bool fromTick)
{
    std::vector<ControllerKind> kinds;
    for (const auto& event : events)
    {
        const auto kind = ControllerKind::of(event);
        if (kind && kind->canRamp() && std::ranges::find(kinds, *kind) == kinds.end())
            kinds.push_back(*kind);
    }
    std::ranges::sort(kinds, kindLess);

    std::vector<LineCrossing> result;
    for (const auto& kind : kinds)
    {
        std::vector<MidiEvent> points;
        std::ranges::copy_if(events, std::back_inserter(points),
                             [&kind](const MidiEvent& e) { return kind.matches(e); });
        std::ranges::stable_sort(points, {}, &MidiEvent::tick);

        const auto isBefore = [tick, fromTick](const MidiEvent& e)
        { return fromTick ? e.tick <= tick : e.tick < tick; };
        const auto afterPrevious = std::ranges::find_if_not(points, isBefore);
        if (afterPrevious == points.begin() || afterPrevious == points.end())
            continue;

        const MidiEvent& previous = *(afterPrevious - 1);
        const MidiEvent& next = *afterPrevious;
        const int previousValue = kind.valueOf(previous);
        const int nextValue = kind.valueOf(next);
        if (!next.ramp || nextValue == previousValue)
            continue;

        const bool nextAtTick = next.tick == tick;
        const int value =
            nextAtTick ? nextValue : rampValueAt(previous.tick, previousValue, next.tick, nextValue, tick);
        result.push_back({kind.point(tick, value), nextAtTick});
    }
    return result;
}
