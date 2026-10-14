#include "model/GridDivision.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("a straight step is a whole note over the denominator", "[model][grid]")
{
    CHECK(GridDivision{1}.stepTicks(480) == 1920);
    CHECK(GridDivision{4}.stepTicks(480) == 480);
    CHECK(GridDivision{16}.stepTicks(480) == 120);
    CHECK(GridDivision{8}.stepTicks(960) == 480);
}

TEST_CASE("the step rounds to the nearest tick and stays at least one tick", "[model][grid]")
{
    CHECK(GridDivision{32}.stepTicks(100) == 13);
    CHECK(GridDivision{32}.stepTicks(1) == 1);
}

TEST_CASE("a triplet step is two thirds of the straight step", "[model][grid]")
{
    CHECK(GridDivision{2, true}.stepTicks(480) == 640);
    CHECK(GridDivision{4, true}.stepTicks(480) == 320);
    CHECK(GridDivision{8, true}.stepTicks(480) == 160);
    CHECK(GridDivision{32, true}.stepTicks(480) == 40);
    CHECK(GridDivision{32, true}.stepTicks(100) == 8);
}
