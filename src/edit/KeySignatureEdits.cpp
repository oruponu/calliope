#include "edit/KeySignatureEdits.h"
#include "notation/KeySignatureName.h"
#include <algorithm>
#include <limits>

void KeySignatureEdits::add(std::vector<KeySignatureChange>& changes, int tick, int sharpsOrFlats, bool isMinor)
{
    sharpsOrFlats = KeySignatureName::normalizeSharpsOrFlats(sharpsOrFlats);

    for (auto& ks : changes)
    {
        if (ks.tick == tick)
        {
            ks.sharpsOrFlats = sharpsOrFlats;
            ks.isMinor = isMinor;
            return;
        }
    }
    changes.push_back({tick, sharpsOrFlats, isMinor});
    std::ranges::sort(changes, {}, &KeySignatureChange::tick);
}

std::vector<KeySignatureChange> KeySignatureEdits::afterMove(const std::vector<KeySignatureChange>& before,
                                                             const std::vector<int>& movedIndices, int anchorIndex,
                                                             int targetTick, const TimelineMap& timeline)
{
    const int count = static_cast<int>(before.size());
    if (anchorIndex < 0 || anchorIndex >= count)
        return before;

    std::vector<bool> isMoving(before.size(), false);
    for (int i : movedIndices)
        if (i >= 0 && i < count)
            isMoving[static_cast<size_t>(i)] = true;
    if (!isMoving[static_cast<size_t>(anchorIndex)])
        return before;

    std::vector<int> bars(before.size());
    for (size_t i = 0; i < before.size(); ++i)
        bars[i] = timeline.tickToBarBeatTick(before[i].tick).bar;

    int prevMovingBar = -1;
    for (int i = 0; i < count; ++i)
    {
        if (!isMoving[static_cast<size_t>(i)])
            continue;
        if (bars[static_cast<size_t>(i)] == prevMovingBar)
            return before;
        prevMovingBar = bars[static_cast<size_t>(i)];
    }

    int clampedTarget = std::max(0, targetTick);
    int targetBar = timeline.tickToBarBeatTick(clampedTarget).bar;
    int targetBarStart = timeline.barStartToTick(targetBar);
    if (clampedTarget - targetBarStart >= timeline.barStartToTick(targetBar + 1) - clampedTarget)
        ++targetBar;
    int delta = targetBar - bars[static_cast<size_t>(anchorIndex)];

    int deltaLo = std::numeric_limits<int>::min();
    int deltaHi = std::numeric_limits<int>::max();
    for (int i = 0; i < count; ++i)
    {
        if (!isMoving[static_cast<size_t>(i)])
            continue;
        if (i == 0)
            deltaLo = std::max(deltaLo, 1 - bars[0]);
        else if (!isMoving[static_cast<size_t>(i - 1)])
            deltaLo = std::max(deltaLo, bars[static_cast<size_t>(i - 1)] + 1 - bars[static_cast<size_t>(i)]);
        if (i + 1 < count && !isMoving[static_cast<size_t>(i + 1)])
        {
            int nextTick = before[static_cast<size_t>(i + 1)].tick;
            int nextBar = bars[static_cast<size_t>(i + 1)];
            int limit = timeline.barStartToTick(nextBar) == nextTick ? nextBar - 1 : nextBar;
            deltaHi = std::min(deltaHi, limit - bars[static_cast<size_t>(i)]);
        }
    }
    if (deltaLo > deltaHi)
        return before;
    delta = std::clamp(delta, deltaLo, deltaHi);

    auto result = before;
    for (int i = 0; i < count; ++i)
        if (isMoving[static_cast<size_t>(i)])
            result[static_cast<size_t>(i)].tick = timeline.barStartToTick(bars[static_cast<size_t>(i)] + delta);
    return result;
}
