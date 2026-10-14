#include "model/TimelineGrid.h"
#include "model/TimeSignatureChange.h"
#include <algorithm>
#include <limits>
#include <ranges>
#include <utility>

namespace
{
constexpr long long intMin = std::numeric_limits<int>::min();
constexpr long long intMax = std::numeric_limits<int>::max();

struct BarSpan
{
    long long start;
    long long end;
    long long beatTicks;
};

// The divisor is always positive.
long long floorDiv(long long dividend, long long divisor)
{
    const long long quotient = dividend / divisor;
    return (dividend % divisor != 0 && dividend < 0) ? quotient - 1 : quotient;
}

BarSpan barAt(const TimelineMap& timeline, long long tick)
{
    const int at = static_cast<int>(std::clamp(tick, 0LL, intMax));
    const TimeSignatureChange signature = timeline.getTimeSignatureAt(at);
    const auto& changes = timeline.getTimeSignatureChanges();
    const auto later = std::ranges::upper_bound(changes, at, {}, &TimeSignatureChange::tick);
    const long long sectionEnd = later == changes.end() ? std::numeric_limits<long long>::max() : later->tick;

    // Beats and bars in int ticks, as TimelineMap measures them.
    const long long beatTicks = std::max(1LL, 4LL * timeline.getTicksPerQuarterNote() / signature.denominator);
    const long long barTicks = beatTicks * std::max(1, signature.numerator);
    const long long start = signature.tick + floorDiv(tick - signature.tick, barTicks) * barTicks;
    return {start, std::min(start + barTicks, sectionEnd), beatTicks};
}
} // namespace

TimelineGrid::TimelineGrid(const TimelineMap& timelineIn, GridDivision divisionIn)
    : timeline(timelineIn), division(divisionIn), stepNum(division.stepNumerator(timeline.getTicksPerQuarterNote())),
      stepDen(division.stepDenominator())
{
    // A step below one tick would put several lines on a tick; a one-tick step already makes every tick a line.
    if (stepDen < 1 || stepNum < stepDen)
    {
        stepNum = 1;
        stepDen = 1;
    }
}

int TimelineGrid::round(int tick) const
{
    const long long earlier = floorLine(tick);
    if (earlier == tick)
        return tick;
    const long long later = nextLine(tick);
    if (later > intMax || (earlier >= intMin && (tick - earlier) * 2 < later - earlier))
        return static_cast<int>(earlier);
    return static_cast<int>(later);
}

int TimelineGrid::floor(int tick) const
{
    const long long line = floorLine(tick);
    return static_cast<int>(line >= intMin ? line : nextLine(tick));
}

std::optional<int> TimelineGrid::next(int tick) const
{
    const long long line = nextLine(tick);
    if (line > intMax)
        return std::nullopt;
    return static_cast<int>(line);
}

std::optional<int> TimelineGrid::previous(int tick) const
{
    const long long line = floorLine(static_cast<long long>(tick) - 1);
    if (line < intMin)
        return std::nullopt;
    return static_cast<int>(line);
}

int TimelineGrid::stepTicks() const
{
    return division.stepTicks(timeline.getTicksPerQuarterNote());
}

std::vector<GridLine> TimelineGrid::lines(int fromTick, int toTick) const
{
    std::vector<GridLine> result;
    std::vector<std::pair<long long, GridLineKind>> inBar;
    for (long long barTick = std::max(fromTick, 0); barTick <= toTick;)
    {
        const BarSpan bar = barAt(timeline, barTick);
        inBar.clear();
        inBar.emplace_back(bar.start, GridLineKind::Bar);
        for (long long k = 1;; ++k)
        {
            const long long tick = bar.start + k * stepNum / stepDen;
            if (tick >= bar.end)
                break;
            inBar.emplace_back(tick, GridLineKind::Sub);
        }
        for (long long tick = bar.start + bar.beatTicks; tick < bar.end; tick += bar.beatTicks)
            inBar.emplace_back(tick, GridLineKind::Beat);

        // Kinds are declared strongest first, so sorting puts the kind to keep first on each tick.
        std::ranges::sort(inBar);
        for (const auto& [tick, kind] : inBar)
            if (tick >= fromTick && tick <= toTick && (result.empty() || result.back().tick != tick))
                result.push_back({static_cast<int>(tick), kind});
        barTick = bar.end;
    }
    return result;
}

long long TimelineGrid::floorLine(long long tick) const
{
    const BarSpan bar = barAt(timeline, tick);
    // The largest k with floor(k * step) <= offset is the largest k with k * stepNum < (offset + 1) * stepDen.
    const long long k = ((tick - bar.start + 1) * stepDen - 1) / stepNum;
    return bar.start + k * stepNum / stepDen;
}

long long TimelineGrid::nextLine(long long tick) const
{
    const BarSpan bar = barAt(timeline, tick);
    const long long k = ((tick - bar.start + 1) * stepDen - 1) / stepNum + 1;
    return std::min(bar.start + k * stepNum / stepDen, bar.end);
}
