#include "edit/ControllerPointEdits.h"
#include "model/ControllerPointConversion.h"
#include <algorithm>
#include <limits>
#include <map>
#include <tuple>

namespace
{
// Among events on one tick, a lower band goes first; points moved right land before the points already there, points
// moved left after them.
enum Band
{
    MovedRight,
    CarriedRight,
    Unmoved,
    CarriedLeft,
    MovedLeft
};

struct Entry
{
    MidiEvent event;
    Band band = Unmoved;
    int order = 0;
    bool selected = false;
};

ControllerPointEditResult arrange(std::vector<Entry> entries)
{
    std::ranges::sort(entries, {}, [](const Entry& e) { return std::tuple{e.event.tick, e.band, e.order}; });
    ControllerPointEditResult result;
    result.events.reserve(entries.size());
    for (const auto& entry : entries)
    {
        if (entry.selected)
            result.selected.insert(static_cast<int>(result.events.size()));
        result.events.push_back(entry.event);
    }
    return result;
}

std::vector<Entry> entriesOf(const std::vector<MidiEvent>& events)
{
    std::vector<Entry> entries;
    entries.reserve(events.size());
    for (int i = 0; i < static_cast<int>(events.size()); ++i)
        entries.push_back({events[static_cast<size_t>(i)], Unmoved, i});
    return entries;
}

void setValueOf(const ControllerKind& kind, MidiEvent& event, int value)
{
    const int clamped = std::clamp(value, 0, kind.maxValue());
    if (kind.type == MidiEvent::Type::PitchBend)
        event.data1 = clamped;
    else
        event.data2 = clamped;
}

bool isValid(const std::vector<MidiEvent>& events, int index)
{
    return index >= 0 && index < static_cast<int>(events.size());
}

std::set<int> pointsOf(const std::vector<MidiEvent>& events, const ControllerKind& kind, const std::set<int>& indices)
{
    std::set<int> points;
    for (int i : indices)
        if (isValid(events, i) && kind.matches(events[static_cast<size_t>(i)]))
            points.insert(i);
    return points;
}

struct Delta
{
    int tick = 0;
    int value = 0;
};

// Held back so that no moved point goes before tick 0 or out of the kind's range.
Delta deltaOf(const std::vector<MidiEvent>& before, const ControllerKind& kind, const std::set<int>& movedPoints,
              const MidiEvent& anchor, int targetTick, int targetValue)
{
    int minTick = std::numeric_limits<int>::max();
    int minValue = std::numeric_limits<int>::max();
    int maxValue = std::numeric_limits<int>::min();
    for (int i : movedPoints)
    {
        const auto& event = before[static_cast<size_t>(i)];
        minTick = std::min(minTick, event.tick);
        minValue = std::min(minValue, kind.valueOf(event));
        maxValue = std::max(maxValue, kind.valueOf(event));
    }
    return {std::max(targetTick - anchor.tick, -minTick),
            std::clamp(targetValue - kind.valueOf(anchor), -minValue, kind.maxValue() - maxValue)};
}

const MidiEvent* pointBefore(const std::vector<Entry>& kept, const ControllerKind& kind, int tick)
{
    const Entry* before = nullptr;
    for (const auto& e : kept)
        if (kind.matches(e.event) && e.event.tick < tick &&
            (before == nullptr || std::tuple{e.event.tick, e.order} > std::tuple{before->event.tick, before->order}))
            before = &e;
    return before != nullptr ? &before->event : nullptr;
}

// For a controller that can ramp. The point the paste follows is converted along so the first step also gets its
// hold point, then dropped.
std::vector<MidiEvent> asRampPoints(const MidiEvent* before, std::vector<MidiEvent> pasted)
{
    if (before != nullptr)
        pasted.insert(pasted.begin(), *before);
    auto converted = convertStepsToPoints(pasted);
    if (before != nullptr)
        converted.erase(converted.begin());
    return converted;
}

// Where the controller cannot ramp, a hold point would be sent as a value of its own, so only the step after it is
// kept. Only points copied where the controller ramps can be hold points; the others are events that are sent.
std::vector<CopiedControllerPoint> withoutHoldPoints(const std::vector<CopiedControllerPoint>& points,
                                                     std::optional<int> valueBefore)
{
    std::vector<CopiedControllerPoint> steps;
    for (size_t n = 0; n < points.size(); ++n)
    {
        const auto previous = n > 0 ? std::optional{points[n - 1].value} : valueBefore;
        const bool hold = points[n].ramp && n + 1 < points.size() && points[n + 1].tick == points[n].tick &&
                          previous == points[n].value;
        if (!hold)
            steps.push_back(points[n]);
    }
    return steps;
}
} // namespace

ControllerPointEditResult ControllerPointEdits::add(const std::vector<MidiEvent>& before, const ControllerKind& kind,
                                                    int tick, int value)
{
    auto entries = entriesOf(before);
    entries.push_back({kind.point(tick, value), Unmoved, static_cast<int>(before.size()), true});
    return arrange(std::move(entries));
}

ControllerPointEditResult ControllerPointEdits::move(const std::vector<MidiEvent>& before, const ControllerKind& kind,
                                                     const std::set<int>& moved, int anchorIndex, int targetTick,
                                                     int targetValue)
{
    const auto movedPoints = pointsOf(before, kind, moved);
    if (!movedPoints.contains(anchorIndex))
        return arrange(entriesOf(before));

    const MidiEvent& anchor = before[static_cast<size_t>(anchorIndex)];
    const auto [deltaTick, deltaValue] = deltaOf(before, kind, movedPoints, anchor, targetTick, targetValue);

    std::set<int> passed;
    for (int j = 0; j < static_cast<int>(before.size()); ++j)
    {
        const auto& event = before[static_cast<size_t>(j)];
        if (movedPoints.contains(j) || !kind.matches(event))
            continue;
        for (int i : movedPoints)
        {
            const int from = before[static_cast<size_t>(i)].tick;
            const int to = from + deltaTick;
            if (event.tick > std::min(from, to) && event.tick < std::max(from, to))
            {
                passed.insert(j);
                break;
            }
        }
    }

    // A single point dragged towards the points before it on its own tick (after it, when dragged right) takes them
    // along, so the step moves whole.
    std::set<int> carried;
    if (movedPoints.size() == 1 && deltaTick != 0)
    {
        for (int j = 0; j < static_cast<int>(before.size()); ++j)
        {
            const auto& event = before[static_cast<size_t>(j)];
            if (j != anchorIndex && event.tick == anchor.tick && kind.matches(event) &&
                (j < anchorIndex) == (deltaTick < 0))
                carried.insert(j);
        }
    }

    // Otherwise a single point reaching or passing points stacked on one tick takes over the step: the far end comes
    // along and stays on its side, the rest of the stack is overwritten.
    if (movedPoints.size() == 1 && deltaTick != 0 && carried.empty())
    {
        const int destination = anchor.tick + deltaTick;
        std::map<int, std::vector<int>> stacks;
        for (int j = 0; j < static_cast<int>(before.size()); ++j)
        {
            const auto& event = before[static_cast<size_t>(j)];
            if (passed.contains(j) || (j != anchorIndex && event.tick == destination && kind.matches(event)))
                stacks[event.tick].push_back(j);
        }
        std::erase_if(stacks, [](const auto& stack) { return stack.second.size() < 2; });
        if (!stacks.empty())
        {
            const auto& nearest = deltaTick > 0 ? stacks.rbegin()->second : stacks.begin()->second;
            passed.insert(nearest.begin(), nearest.end());
            carried.insert(deltaTick > 0 ? nearest.back() : nearest.front());
        }
    }

    const Band movedBand = deltaTick > 0 ? MovedRight : deltaTick < 0 ? MovedLeft : Unmoved;
    std::vector<Entry> entries;
    for (int j = 0; j < static_cast<int>(before.size()); ++j)
    {
        Entry entry{before[static_cast<size_t>(j)], Unmoved, j};
        if (movedPoints.contains(j))
        {
            entry.event.tick += deltaTick;
            setValueOf(kind, entry.event, kind.valueOf(entry.event) + deltaValue);
            entry.band = movedBand;
            entry.selected = true;
        }
        else if (carried.contains(j))
        {
            entry.event.tick = anchor.tick + deltaTick;
            entry.band = deltaTick > 0 ? CarriedRight : CarriedLeft;
        }
        else if (passed.contains(j))
        {
            continue;
        }
        entries.push_back(entry);
    }
    return arrange(std::move(entries));
}

std::optional<MidiEvent> ControllerPointEdits::movedAnchor(const std::vector<MidiEvent>& before,
                                                           const ControllerKind& kind, const std::set<int>& moved,
                                                           int anchorIndex, int targetTick, int targetValue)
{
    const auto movedPoints = pointsOf(before, kind, moved);
    if (!movedPoints.contains(anchorIndex))
        return std::nullopt;

    auto anchor = before[static_cast<size_t>(anchorIndex)];
    const auto delta = deltaOf(before, kind, movedPoints, anchor, targetTick, targetValue);
    anchor.tick += delta.tick;
    setValueOf(kind, anchor, kind.valueOf(anchor) + delta.value);
    return anchor;
}

ControllerPointEditResult ControllerPointEdits::remove(const std::vector<MidiEvent>& before,
                                                       const std::set<int>& removed)
{
    auto entries = entriesOf(before);
    std::erase_if(entries, [&removed](const Entry& e) { return removed.contains(e.order); });
    return arrange(std::move(entries));
}

ControllerPointEditResult ControllerPointEdits::setValue(const std::vector<MidiEvent>& before,
                                                         const ControllerKind& kind, int index, int value)
{
    auto entries = entriesOf(before);
    if (isValid(before, index) && kind.matches(before[static_cast<size_t>(index)]))
    {
        auto& entry = entries[static_cast<size_t>(index)];
        setValueOf(kind, entry.event, value);
        entry.selected = true;
    }
    return arrange(std::move(entries));
}

std::set<int> ControllerPointEdits::allPoints(const std::vector<MidiEvent>& events, const ControllerKind& kind)
{
    std::set<int> points;
    for (int i = 0; i < static_cast<int>(events.size()); ++i)
        if (kind.matches(events[static_cast<size_t>(i)]))
            points.insert(i);
    return points;
}

CopiedControllerPoints ControllerPointEdits::copy(const std::vector<MidiEvent>& events, const ControllerKind& kind,
                                                  const std::set<int>& selected)
{
    CopiedControllerPoints copied{kind, {}};
    for (int i : pointsOf(events, kind, selected))
    {
        const auto& event = events[static_cast<size_t>(i)];
        copied.points.push_back({event.tick, kind.valueOf(event), event.ramp});
    }
    if (!copied.points.empty())
    {
        const int firstTick = copied.points.front().tick;
        for (auto& point : copied.points)
            point.tick -= firstTick;
    }
    return copied;
}

ControllerPointEditResult ControllerPointEdits::paste(const std::vector<MidiEvent>& before, const ControllerKind& kind,
                                                      const std::vector<CopiedControllerPoint>& points, int atTick)
{
    auto entries = entriesOf(before);
    if (points.empty())
        return arrange(std::move(entries));

    const int firstTick = atTick + points.front().tick;
    const int lastTick = atTick + points.back().tick;
    // A pasted point takes the place of a replaced one on its tick, so it keeps its order among other events there.
    std::map<int, std::vector<int>> replacedOrders;
    std::erase_if(entries,
                  [&](const Entry& e)
                  {
                      const bool replaced =
                          kind.matches(e.event) && e.event.tick >= firstTick && e.event.tick <= lastTick;
                      if (replaced)
                          replacedOrders[e.event.tick].push_back(e.order);
                      return replaced;
                  });
    const MidiEvent* followed = pointBefore(entries, kind, firstTick);
    const auto kept =
        kind.canRamp()
            ? points
            : withoutHoldPoints(points, followed != nullptr ? std::optional{kind.valueOf(*followed)} : std::nullopt);
    std::vector<MidiEvent> pasted;
    for (const auto& point : kept)
    {
        auto event = kind.point(atTick + point.tick, point.value);
        event.ramp = kind.canRamp() && point.ramp;
        pasted.push_back(event);
    }
    if (kind.canRamp())
        pasted = asRampPoints(followed, std::move(pasted));
    for (int i = 0; i < static_cast<int>(pasted.size()); ++i)
    {
        const auto& event = pasted[static_cast<size_t>(i)];
        int order = static_cast<int>(before.size()) + i;
        if (auto& orders = replacedOrders[event.tick]; !orders.empty())
        {
            order = orders.front();
            orders.erase(orders.begin());
        }
        entries.push_back({event, Unmoved, order, true});
    }
    return arrange(std::move(entries));
}
