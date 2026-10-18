#include "engine/CountInBars.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("count-in bars are clamped into one to four", "[engine][count-in]")
{
    CHECK(CountInBars::clamped(1) == 1);
    CHECK(CountInBars::clamped(3) == 3);
    CHECK(CountInBars::clamped(4) == 4);
    CHECK(CountInBars::clamped(0) == 1);
    CHECK(CountInBars::clamped(-3) == 1);
    CHECK(CountInBars::clamped(5) == 4);
}
