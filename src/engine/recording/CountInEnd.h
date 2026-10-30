#pragma once

#include "engine/transport/LoopRange.h"

// Ends the count-in: count-in input queued before it is recorded at startTick.
struct CountInEnd
{
    int startTick = 0;
    // The clock's loop when the count-in ended, for input quantize.
    LoopRange loop;

    bool operator==(const CountInEnd&) const = default;
};
