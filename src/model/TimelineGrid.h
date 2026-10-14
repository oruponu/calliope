#pragma once

#include "model/GridDivision.h"
#include "model/GridLine.h"
#include "model/TimelineMap.h"
#include <optional>
#include <vector>

// Grid lines counted afresh from every bar start. A bar holds start + floor(k * step) for each k that stays inside it,
// and the next bar start is always a line, so a bar the step does not divide ends with one shorter step. Every time
// signature change starts a bar, and ticks before 0 continue the time signature at tick 0 backwards.
// Holds the timeline by reference: build one where it is used and do not keep it.
class TimelineGrid
{
public:
    TimelineGrid(const TimelineMap& timeline, GridDivision division);
    // For edits that do not snap.
    static TimelineGrid everyTick(const TimelineMap& timeline);

    // The nearest line, the later one on a tie. When only one of the two lines fits in int, that one.
    int round(int tick) const;
    // The last line at or before the tick; the first line after it when no such line fits in int.
    int floor(int tick) const;
    // The nearest line strictly after or before the tick, or nullopt when that line is outside int.
    std::optional<int> next(int tick) const;
    std::optional<int> previous(int tick) const;
    int stepTicks() const;
    // Grid lines and beat starts from max(fromTick, 0) through toTick in ascending order. A beat start is listed even
    // when it is not a grid line, and a tick that is both keeps the stronger kind.
    std::vector<GridLine> lines(int fromTick, int toTick) const;

private:
    // The step is stepNum / stepDen ticks.
    TimelineGrid(const TimelineMap& timeline, long long stepNum, long long stepDen);

    long long floorLine(long long tick) const;
    long long nextLine(long long tick) const;

    const TimelineMap& timeline;
    long long stepNum;
    long long stepDen;
};
