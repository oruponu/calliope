#include "edit/TempoEdits.h"
#include "model/TimelineMap.h"
#include <algorithm>
#include <limits>

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

std::vector<TempoChange> TempoEdits::afterMove(const std::vector<TempoChange>& before,
                                               const std::vector<int>& movedIndices, int anchorIndex, int targetTick,
                                               double targetBpm, int gridTicks)
{
    const int count = static_cast<int>(before.size());
    if (anchorIndex < 0 || anchorIndex >= count)
        return before;

    std::set<int> movingTicks;
    for (int i : movedIndices)
        if (i >= 0 && i < count && before[static_cast<size_t>(i)].tick != 0)
            movingTicks.insert(i);

    const TempoChange& anchor = before[static_cast<size_t>(anchorIndex)];
    int deltaTick = (anchor.tick == 0) ? 0 : targetTick - anchor.tick;

    int deltaLo = std::numeric_limits<int>::min();
    int deltaHi = std::numeric_limits<int>::max();
    for (int i : movingTicks)
        deltaLo = std::max(deltaLo, gridTicks - before[static_cast<size_t>(i)].tick);
    for (int i = 0; i + 1 < count; ++i)
    {
        const bool aMoving = movingTicks.contains(i);
        const bool bMoving = movingTicks.contains(i + 1);
        const int gap = before[static_cast<size_t>(i + 1)].tick - before[static_cast<size_t>(i)].tick;
        if (bMoving && !aMoving)
            deltaLo = std::max(deltaLo, gridTicks - gap);
        else if (aMoving && !bMoving)
            deltaHi = std::min(deltaHi, gap - gridTicks);
    }
    deltaTick = (deltaLo > deltaHi) ? 0 : std::clamp(deltaTick, deltaLo, deltaHi);

    double groupMinBpm = TimelineMap::maxBpm;
    double groupMaxBpm = TimelineMap::minBpm;
    for (int i : movedIndices)
        if (i >= 0 && i < count)
        {
            groupMinBpm = std::min(groupMinBpm, before[static_cast<size_t>(i)].bpm);
            groupMaxBpm = std::max(groupMaxBpm, before[static_cast<size_t>(i)].bpm);
        }
    const double deltaBpm =
        std::clamp(targetBpm - anchor.bpm, TimelineMap::minBpm - groupMinBpm, TimelineMap::maxBpm - groupMaxBpm);

    auto after = before;
    for (int i : movedIndices)
        if (i >= 0 && i < count)
            after[static_cast<size_t>(i)].bpm = before[static_cast<size_t>(i)].bpm + deltaBpm;
    for (int i : movingTicks)
        after[static_cast<size_t>(i)].tick = before[static_cast<size_t>(i)].tick + deltaTick;
    return after;
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
