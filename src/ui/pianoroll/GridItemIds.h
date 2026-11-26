#pragma once

#include "model/grid/GridDivision.h"

// A straight grid item's id is its denominator, and a triplet's is this offset plus its denominator.
namespace GridItemIds
{
constexpr int tripletOffset = 100;

constexpr int forDivision(GridDivision division)
{
    return division.denominator + (division.triplet ? tripletOffset : 0);
}

constexpr GridDivision toDivision(int id)
{
    if (id > tripletOffset)
        return {id - tripletOffset, true};
    return {id};
}
} // namespace GridItemIds
