#include "engine/InputQuantize.h"
#include <algorithm>

std::optional<int> quantizeInputStart(int playedTick, const TimelineGrid& grid, LoopRange loop)
{
    const int rounded = grid.round(playedTick);
    const bool insideLoop = loop.enabled && loop.start < loop.end && playedTick >= loop.start && playedTick < loop.end;
    if (!insideLoop)
        return rounded;
    if (rounded >= loop.end)
        return std::nullopt;
    return std::max(rounded, loop.start);
}
