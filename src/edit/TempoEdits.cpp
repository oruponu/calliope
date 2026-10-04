#include "edit/TempoEdits.h"
#include <algorithm>

int TempoEdits::add(std::vector<TempoChange>& changes, int tick, double bpm)
{
    for (size_t i = 0; i < changes.size(); ++i)
    {
        if (changes[i].tick == tick)
        {
            changes[i].bpm = bpm;
            return static_cast<int>(i);
        }
    }
    changes.push_back({tick, bpm});
    std::ranges::sort(changes, {}, &TempoChange::tick);
    auto it = std::ranges::find(changes, tick, &TempoChange::tick);
    return static_cast<int>(it - changes.begin());
}

std::vector<TempoChange> TempoEdits::afterDelete(const std::vector<TempoChange>& before,
                                                 const std::set<int>& deletedIndices)
{
    std::vector<TempoChange> after;
    after.reserve(before.size());
    for (int i = 0; i < static_cast<int>(before.size()); ++i)
    {
        const auto& change = before[static_cast<size_t>(i)];
        if (!deletedIndices.contains(i) || change.tick == 0)
            after.push_back(change);
    }
    return after;
}

std::vector<TempoChange> TempoEdits::afterPaste(const std::vector<TempoChange>& before,
                                                const std::vector<TempoChange>& items, int anchorTick)
{
    auto after = before;
    for (const auto& item : items)
        add(after, anchorTick + item.tick, item.bpm);
    return after;
}
