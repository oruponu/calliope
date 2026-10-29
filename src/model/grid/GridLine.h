#pragma once

#include "model/grid/GridLineKind.h"

struct GridLine
{
    int tick;
    GridLineKind kind;
    bool operator==(const GridLine&) const = default;
};
