#include "model/timeline/TempoChangeList.h"
#include <algorithm>

int TempoChangeList::put(std::vector<TempoChange>& changes, int tick, double bpm)
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
