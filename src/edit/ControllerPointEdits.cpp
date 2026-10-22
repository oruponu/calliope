#include "edit/ControllerPointEdits.h"
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
    std::set<int> movedPoints;
    for (int i : moved)
        if (isValid(before, i) && kind.matches(before[static_cast<size_t>(i)]))
            movedPoints.insert(i);
    if (!movedPoints.contains(anchorIndex))
        return arrange(entriesOf(before));

    const MidiEvent& anchor = before[static_cast<size_t>(anchorIndex)];
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
    const int deltaTick = std::max(targetTick - anchor.tick, -minTick);
    const int deltaValue = std::clamp(targetValue - kind.valueOf(anchor), -minValue, kind.maxValue() - maxValue);

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

ControllerPointEditResult ControllerPointEdits::remove(const std::vector<MidiEvent>& before,
                                                       const std::set<int>& removed)
{
    auto entries = entriesOf(before);
    std::erase_if(entries, [&removed](const Entry& e) { return removed.contains(e.order); });
    return arrange(std::move(entries));
}
