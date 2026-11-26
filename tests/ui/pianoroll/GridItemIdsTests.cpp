#include "ui/pianoroll/GridItemIds.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("a straight grid's item id is its denominator")
{
    CHECK(GridItemIds::forDivision({16, false}) == 16);
    CHECK(GridItemIds::toDivision(16) == GridDivision{16, false});
    CHECK(GridItemIds::toDivision(1) == GridDivision{1, false});
}

TEST_CASE("a triplet grid's item id is the offset plus its denominator")
{
    CHECK(GridItemIds::forDivision({8, true}) == 108);
    CHECK(GridItemIds::toDivision(108) == GridDivision{8, true});
    CHECK(GridItemIds::toDivision(102) == GridDivision{2, true});
}
