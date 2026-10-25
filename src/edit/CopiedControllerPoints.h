#pragma once

#include "model/ControllerKind.h"
#include <optional>
#include <vector>

struct CopiedControllerPoint
{
    // Counted from the first copied point.
    int tick = 0;
    int value = 0;
    bool ramp = false;
    // Only program change points use these. A missing value means that the group had no bank select of that number.
    std::optional<int> bankMsb;
    std::optional<int> bankLsb;

    bool operator==(const CopiedControllerPoint&) const = default;
};

struct CopiedControllerPoints
{
    ControllerKind kind;
    std::vector<CopiedControllerPoint> points;

    bool operator==(const CopiedControllerPoints&) const = default;
};
