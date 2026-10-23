#include "model/ControllerRamps.h"
#include "model/ControllerKind.h"
#include "model/ControllerPointConversion.h"
#include <algorithm>
#include <cstdlib>
#include <map>
#include <utility>

namespace
{
// The (tick, value) pairs strictly between the two points where the rounded value changes. The work grows with the
// smaller of the tick span and the value change, so a long span with a small change stays cheap.
std::vector<std::pair<int, int>> rampSteps(int t0, int v0, int t1, int v1)
{
    std::vector<std::pair<int, int>> steps;
    const long long span = static_cast<long long>(t1) - t0;
    const long long change = v1 - v0;
    if (span < 2 || change == 0)
        return steps;

    if (span - 1 <= std::abs(change))
    {
        int last = v0;
        for (long long offset = 1; offset < span; ++offset)
            if (const int value = rampValueAt(t0, v0, t1, v1, static_cast<int>(t0 + offset)); value != last)
            {
                steps.push_back({static_cast<int>(t0 + offset), value});
                last = value;
            }
        return steps;
    }

    const int direction = change > 0 ? 1 : -1;
    for (int value = v0 + direction; value != v1 + direction; value += direction)
    {
        // The first offset at which the rounded value reaches value.
        const long long numerator = (2LL * std::abs(value - v0) - 1) * span;
        const long long denominator = 2 * std::abs(change);
        const long long offset =
            direction > 0 ? (numerator + denominator - 1) / denominator : numerator / denominator + 1;
        if (offset >= span)
            break;
        if (!steps.empty() && steps.back().first == t0 + offset)
            steps.back().second = value;
        else
            steps.push_back({static_cast<int>(t0 + offset), value});
    }
    return steps;
}
} // namespace

int rampValueAt(int fromTick, int fromValue, int toTick, int toValue, int tick)
{
    const long long span = static_cast<long long>(toTick) - fromTick;
    const long long change = toValue - fromValue;
    const long long offset = static_cast<long long>(tick) - fromTick;
    // Values are never negative, so rounding half away from zero is floor(x + 0.5), computed exactly.
    return static_cast<int>((2 * fromValue * span + 2 * change * offset + span) / (2 * span));
}

std::vector<MidiEvent> expandControllerRamps(const std::vector<MidiEvent>& events)
{
    auto ordered = events;
    std::ranges::stable_sort(ordered, {}, &MidiEvent::tick);

    std::vector<MidiEvent> expanded;
    expanded.reserve(ordered.size());
    std::map<std::pair<MidiEvent::Type, int>, MidiEvent> previous;
    const auto unsent = unsentControllerPoints(ordered);
    for (size_t i = 0; i < ordered.size(); ++i)
    {
        auto event = ordered[i];
        const auto kind = ControllerKind::of(event);
        const bool ramp = event.ramp && kind.has_value() && kind->canRamp();
        event.ramp = false;
        if (!kind.has_value())
        {
            expanded.push_back(event);
            continue;
        }

        const std::pair key{kind->type, kind->number};
        if (const auto it = previous.find(key); ramp && it != previous.end())
        {
            for (const auto& [tick, value] :
                 rampSteps(it->second.tick, kind->valueOf(it->second), event.tick, kind->valueOf(event)))
            {
                expanded.push_back(kind->point(tick, value));
                expanded.back().ramp = false;
            }
        }
        if (!unsent[i])
            expanded.push_back(event);
        previous.insert_or_assign(key, event);
    }

    std::ranges::stable_sort(expanded, {}, &MidiEvent::tick);
    return expanded;
}
