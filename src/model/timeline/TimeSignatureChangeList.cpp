#include "model/timeline/TimeSignatureChangeList.h"
#include "model/timeline/TimelineMap.h"
#include <algorithm>
#include <ranges>

namespace
{
int ticksPerBarOf(const TimeSignatureChange& ts, int ppq)
{
    return ppq * 4 / ts.denominator * ts.numerator;
}
} // namespace

void TimeSignatureChangeList::put(std::vector<TimeSignatureChange>& changes, int tick, int numerator, int denominator,
                                  int ppq)
{
    TimelineMap timeline;
    timeline.setTicksPerQuarterNote(ppq);
    timeline.setTimeSignatureChanges(changes);

    auto bars = changes |
                std::views::transform([&timeline](const TimeSignatureChange& ts)
                                      { return timeline.tickToBarBeatTick(ts.tick).bar; }) |
                std::ranges::to<std::vector>();

    const int targetBar = timeline.tickToBarBeatTick(tick).bar;
    const auto sameBar = std::ranges::find_last(bars, targetBar);
    if (!sameBar.empty())
    {
        auto& ts = changes[static_cast<size_t>(sameBar.begin() - bars.begin())];
        ts.numerator = numerator;
        ts.denominator = denominator;
    }
    else
    {
        size_t insertPos = 0;
        while (insertPos < changes.size() && changes[insertPos].tick < tick)
            ++insertPos;
        changes.insert(changes.begin() + insertPos, {tick, numerator, denominator});
        bars.insert(bars.begin() + insertPos, targetBar);
    }

    if (!changes.empty())
        changes[0].tick = 0;
    retick(changes, bars, 1, ppq);
}

std::vector<int> TimeSignatureChangeList::barNumbers(const std::vector<TimeSignatureChange>& changes, int ppq)
{
    std::vector<int> bars(changes.size());
    if (bars.empty())
        return bars;
    bars[0] = 1;
    for (size_t i = 1; i < changes.size(); ++i)
        bars[i] = bars[i - 1] + (changes[i].tick - changes[i - 1].tick) / ticksPerBarOf(changes[i - 1], ppq);
    return bars;
}

void TimeSignatureChangeList::retick(std::vector<TimeSignatureChange>& changes, const std::vector<int>& bars,
                                     std::size_t first, int ppq)
{
    for (size_t i = first; i < changes.size(); ++i)
        changes[i].tick = changes[i - 1].tick + (bars[i] - bars[i - 1]) * ticksPerBarOf(changes[i - 1], ppq);
}
