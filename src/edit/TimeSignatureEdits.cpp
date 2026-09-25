#include "edit/TimeSignatureEdits.h"
#include "model/TimelineMap.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
int ticksPerBarOf(const TimeSignatureChange& ts, int ppq)
{
    return ppq * 4 / ts.denominator * ts.numerator;
}

// assumes the first change is at bar 1
std::vector<int> barNumbersOf(const std::vector<TimeSignatureChange>& changes, int ppq)
{
    std::vector<int> bars(changes.size());
    if (bars.empty())
        return bars;
    bars[0] = 1;
    for (size_t i = 1; i < changes.size(); ++i)
        bars[i] = bars[i - 1] + (changes[i].tick - changes[i - 1].tick) / ticksPerBarOf(changes[i - 1], ppq);
    return bars;
}

void retickFrom(std::vector<TimeSignatureChange>& changes, const std::vector<int>& bars, size_t first, int ppq)
{
    for (size_t i = first; i < changes.size(); ++i)
        changes[i].tick = changes[i - 1].tick + (bars[i] - bars[i - 1]) * ticksPerBarOf(changes[i - 1], ppq);
}
} // namespace

void TimeSignatureEdits::add(std::vector<TimeSignatureChange>& changes, int tick, int num, int den, int ppq)
{
    TimelineMap timeline;
    timeline.setTicksPerQuarterNote(ppq);
    timeline.setTimeSignatureChanges(changes);

    std::vector<int> bars;
    bars.reserve(changes.size());
    for (const auto& ts : changes)
        bars.push_back(timeline.tickToBarBeatTick(ts.tick).bar);

    bool modified = false;
    for (auto& ts : changes)
    {
        if (ts.tick == tick)
        {
            ts.numerator = num;
            ts.denominator = den;
            modified = true;
            break;
        }
    }

    if (!modified)
    {
        int targetBar = timeline.tickToBarBeatTick(tick).bar;
        size_t insertPos = 0;
        while (insertPos < changes.size() && changes[insertPos].tick < tick)
            ++insertPos;
        changes.insert(changes.begin() + insertPos, {tick, num, den});
        bars.insert(bars.begin() + insertPos, targetBar);
    }

    if (!changes.empty())
        changes[0].tick = 0;
    retickFrom(changes, bars, 1, ppq);
}

std::vector<TimeSignatureChange> TimeSignatureEdits::afterMove(const std::vector<TimeSignatureChange>& before,
                                                               const std::vector<int>& movedIndices, int anchorIndex,
                                                               int targetTick, int ppq)
{
    const int count = static_cast<int>(before.size());
    if (anchorIndex <= 0 || anchorIndex >= count)
        return before;

    std::vector<bool> isMoving(before.size(), false);
    int firstMoving = count;
    for (int i : movedIndices)
        if (i >= 1 && i < count)
        {
            isMoving[static_cast<size_t>(i)] = true;
            firstMoving = std::min(firstMoving, i);
        }
    if (!isMoving[static_cast<size_t>(anchorIndex)])
        return before;

    const auto bars = barNumbersOf(before, ppq);

    auto rebuildFromFirstMoving = [&](const std::vector<int>& barNumbers)
    {
        auto result = before;
        retickFrom(result, barNumbers, static_cast<size_t>(firstMoving), ppq);
        return result;
    };
    auto shiftedBars = [&](int delta)
    {
        auto barNumbers = bars;
        for (size_t i = 0; i < barNumbers.size(); ++i)
            if (isMoving[i])
                barNumbers[i] += delta;
        return barNumbers;
    };

    auto base = rebuildFromFirstMoving(bars);
    const int anchorTick0 = base[static_cast<size_t>(anchorIndex)].tick;
    const int slope = rebuildFromFirstMoving(shiftedBars(1))[static_cast<size_t>(anchorIndex)].tick - anchorTick0;
    if (slope <= 0)
        return before;

    int delta = static_cast<int>(std::floor((targetTick - anchorTick0) / static_cast<double>(slope) + 0.5));

    int deltaLo = std::numeric_limits<int>::min();
    int deltaHi = std::numeric_limits<int>::max();
    for (int i = 0; i + 1 < count; ++i)
    {
        bool aMoving = isMoving[static_cast<size_t>(i)];
        bool bMoving = isMoving[static_cast<size_t>(i + 1)];
        int gapBars = bars[static_cast<size_t>(i + 1)] - bars[static_cast<size_t>(i)];
        if (bMoving && !aMoving)
            deltaLo = std::max(deltaLo, 1 - gapBars);
        else if (aMoving && !bMoving)
            deltaHi = std::min(deltaHi, gapBars - 1);
    }
    delta = (deltaLo > deltaHi) ? 0 : std::clamp(delta, deltaLo, deltaHi);

    if (delta == 0)
        return base;
    return rebuildFromFirstMoving(shiftedBars(delta));
}

std::vector<TimeSignatureChange> TimeSignatureEdits::afterDelete(const std::vector<TimeSignatureChange>& before,
                                                                 const std::set<int>& deletedIndices, int ppq)
{
    if (before.empty())
        return before;

    const auto bars = barNumbersOf(before, ppq);

    std::vector<TimeSignatureChange> result;
    std::vector<int> resultBars;
    result.reserve(before.size());
    resultBars.reserve(before.size());
    for (size_t i = 0; i < before.size(); ++i)
    {
        const bool remove = i > 0 && deletedIndices.count(static_cast<int>(i)) > 0;
        if (!remove)
        {
            result.push_back(before[i]);
            resultBars.push_back(bars[i]);
        }
    }

    if (result.size() == before.size())
        return before;

    retickFrom(result, resultBars, 1, ppq);
    return result;
}

std::vector<TimeSignatureChange> TimeSignatureEdits::afterPaste(const std::vector<TimeSignatureChange>& before,
                                                                const std::vector<RelativeTimeSignature>& items,
                                                                int anchorBar, int ppq)
{
    if (before.empty() || items.empty())
        return before;

    const auto bars = barNumbersOf(before, ppq);

    auto result = before;
    auto resultBars = bars;
    for (const auto& item : items)
    {
        const int bar = anchorBar + item.barOffset;
        if (bar < 1)
            continue;
        auto it = std::ranges::lower_bound(resultBars, bar);
        const auto pos = it - resultBars.begin();
        if (it != resultBars.end() && *it == bar)
        {
            result[static_cast<size_t>(pos)].numerator = item.numerator;
            result[static_cast<size_t>(pos)].denominator = item.denominator;
        }
        else
        {
            result.insert(result.begin() + pos, {0, item.numerator, item.denominator});
            resultBars.insert(it, bar);
        }
    }

    retickFrom(result, resultBars, 1, ppq);
    return result;
}
