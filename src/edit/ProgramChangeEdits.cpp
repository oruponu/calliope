#include "edit/ProgramChangeEdits.h"
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
