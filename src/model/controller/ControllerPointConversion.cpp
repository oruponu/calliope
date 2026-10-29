#include "model/controller/ControllerPointConversion.h"
#include "model/controller/ControllerKind.h"
#include <algorithm>
#include <map>
#include <numeric>
#include <utility>

namespace
{
using KindKey = std::pair<MidiEvent::Type, int>;

KindKey keyOf(const ControllerKind& kind)
{
    return {kind.type, kind.number};
}
} // namespace

std::vector<MidiEvent> convertStepsToPoints(const std::vector<MidiEvent>& events)
{
    auto ordered = events;
    std::ranges::stable_sort(ordered, {}, &MidiEvent::tick);

    std::vector<MidiEvent> converted;
    converted.reserve(ordered.size());
    std::map<KindKey, MidiEvent> previous;
    for (auto event : ordered)
    {
        const auto kind = ControllerKind::of(event);
        if (!kind.has_value() || !kind->canRamp())
        {
            converted.push_back(event);
            continue;
        }

        const auto key = keyOf(*kind);
        if (const auto it = previous.find(key); !event.ramp && it != previous.end())
        {
            const auto& from = it->second;
            const int heldValue = kind->valueOf(from);
            // Wide enough for the ticks of a project file that is not validated yet.
            if (heldValue != kind->valueOf(event) && static_cast<long long>(event.tick) - from.tick >= 2)
                converted.push_back(kind->point(event.tick, heldValue));
        }
        event.ramp = true;
        converted.push_back(event);
        previous.insert_or_assign(key, event);
    }
    return converted;
}

std::vector<MidiEvent> convertHoldsToSteps(const std::vector<MidiEvent>& events)
{
    auto ordered = events;
    std::ranges::stable_sort(ordered, {}, &MidiEvent::tick);
    const auto holds = unsentControllerPoints(ordered);

    std::vector<MidiEvent> steps;
    steps.reserve(ordered.size());
    std::map<KindKey, bool> switchPending;
    for (size_t i = 0; i < ordered.size(); ++i)
    {
        auto event = ordered[i];
        const auto kind = ControllerKind::of(event);
        if (!kind.has_value() || !kind->canRamp())
        {
            steps.push_back(event);
            continue;
        }
        bool& pending = switchPending[keyOf(*kind)];
        if (holds[i])
        {
            pending = true;
            continue;
        }
        if (pending)
        {
            event.ramp = false;
            pending = false;
        }
        steps.push_back(event);
    }
    return steps;
}

std::vector<bool> unsentControllerPoints(const std::vector<MidiEvent>& events)
{
    std::vector<int> order(events.size());
    std::iota(order.begin(), order.end(), 0);
    std::ranges::stable_sort(order, {}, [&events](int i) { return events[static_cast<size_t>(i)].tick; });

    std::map<KindKey, std::vector<int>> byKind;
    for (int i : order)
        if (const auto kind = ControllerKind::of(events[static_cast<size_t>(i)]); kind && kind->canRamp())
            byKind[keyOf(*kind)].push_back(i);

    std::vector<bool> unsent(events.size(), false);
    for (const auto& [key, indices] : byKind)
    {
        const auto kind = ControllerKind::of(events[static_cast<size_t>(indices.front())]);
        for (size_t n = 1; n + 1 < indices.size(); ++n)
        {
            const auto& before = events[static_cast<size_t>(indices[n - 1])];
            const auto& event = events[static_cast<size_t>(indices[n])];
            const auto& after = events[static_cast<size_t>(indices[n + 1])];
            if (after.tick == event.tick && kind->valueOf(before) == kind->valueOf(event))
                unsent[static_cast<size_t>(indices[n])] = true;
        }
    }
    return unsent;
}
