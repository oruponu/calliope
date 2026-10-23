#pragma once

#include "model/ControllerKind.h"
#include <vector>

struct CopiedControllerPoint
{
    // Counted from the first copied point.
    int tick = 0;
    int value = 0;
    bool ramp = false;

    bool operator==(const CopiedControllerPoint&) const = default;
};

struct CopiedControllerPoints
{
    ControllerKind kind;
    std::vector<CopiedControllerPoint> points;

    bool operator==(const CopiedControllerPoints&) const = default;
};
