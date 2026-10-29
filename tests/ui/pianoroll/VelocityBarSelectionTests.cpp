#include "ui/pianoroll/VelocityBarSelection.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("bars inside or across the range are selected", "[ui][velocity]")
{
    const std::vector<VelocityBar> bars{{100, 0}, {150, 0}, {196, 0}};
    CHECK(VelocityBarSelection::overlapping(bars, 95, 200, 7) == std::vector<int>{0, 1, 2});
    CHECK(VelocityBarSelection::overlapping(bars, 104, 150, 7) == std::vector<int>{0, 1});
}

TEST_CASE("bars beside the range are not selected", "[ui][velocity]")
{
    const std::vector<VelocityBar> bars{{100, 0}, {200, 0}};
    CHECK(VelocityBarSelection::overlapping(bars, 107, 199, 7).empty());
}

TEST_CASE("a range of no width selects the bar under it", "[ui][velocity]")
{
    const std::vector<VelocityBar> bars{{100, 0}, {200, 0}};
    CHECK(VelocityBarSelection::overlapping(bars, 106, 106, 7) == std::vector<int>{0});
    CHECK(VelocityBarSelection::overlapping(bars, 150, 150, 7).empty());
}

TEST_CASE("bars on one tick are selected together", "[ui][velocity]")
{
    CHECK(VelocityBarSelection::overlapping({{100, 10}, {100, 60}}, 102, 102, 7) == std::vector<int>{0, 1});
}
