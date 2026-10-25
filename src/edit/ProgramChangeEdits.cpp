#include "edit/ProgramChangeEdits.h"
#include "model/ControllerKind.h"
#include <algorithm>
#include <ranges>

namespace
{
std::vector<int> programChangeIndices(const std::vector<MidiEvent>& events)
{
    std::vector<int> indices;
    for (int i = 0; i < static_cast<int>(events.size()); ++i)
        if (events[static_cast<size_t>(i)].type == MidiEvent::Type::ProgramChange)
            indices.push_back(i);
    std::ranges::stable_sort(indices, {}, [&events](int i) { return events[static_cast<size_t>(i)].tick; });
    return indices;
}

struct Entry
{
    MidiEvent event;
    bool selected = false;
};
} // namespace

std::optional<int> ProgramChangeEdits::blockTickAt(const std::vector<MidiEvent>& events, int tick)
{
    std::optional<int> blockTick;
    for (int i : programChangeIndices(events))
    {
        const int eventTick = events[static_cast<size_t>(i)].tick;
        if (eventTick > tick)
            break;
        blockTick = eventTick;
    }
    return blockTick;
}

std::set<int> ProgramChangeEdits::onTick(const std::vector<MidiEvent>& events, int tick)
{
    std::set<int> indices;
    for (int i : programChangeIndices(events))
    {
        const int eventTick = events[static_cast<size_t>(i)].tick;
        if (eventTick > tick)
            break;
        if (eventTick == tick)
            indices.insert(i);
    }
    return indices;
}

int ProgramChangeEdits::programAt(const std::vector<MidiEvent>& events, int tick)
{
    int program = 0;
    for (int i : programChangeIndices(events))
    {
        const auto& event = events[static_cast<size_t>(i)];
        if (event.tick > tick)
            break;
        program = event.data1;
    }
    return program;
}

ControllerPointEditResult ProgramChangeEdits::place(const std::vector<MidiEvent>& before, int tick, int program)
{
    const auto kind = ControllerKind::programChange();
    const auto replaced = onTick(before, tick);
    const int kept = replaced.empty() ? -1 : *replaced.rbegin();

    std::vector<Entry> entries;
    entries.reserve(before.size() + (kept < 0 ? 1 : 0));
    for (int i = 0; i < static_cast<int>(before.size()); ++i)
    {
        if (replaced.contains(i) && i != kept)
            continue;
        MidiEvent event = before[static_cast<size_t>(i)];
        const bool selected = i == kept;
        if (selected)
            event = kind.point(tick, program);
        entries.push_back({event, selected});
    }
    if (kept < 0)
        entries.push_back({kind.point(tick, program), true});

    std::ranges::stable_sort(entries, {}, [](const Entry& entry) { return entry.event.tick; });
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
