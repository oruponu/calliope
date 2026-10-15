#include "ui/RowSelection.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("no rows make an empty selection", "[ui][row-selection]")
{
    CHECK(RowSelection::fromRows({}).isEmpty());
}

TEST_CASE("consecutive rows make one range", "[ui][row-selection]")
{
    const auto set = RowSelection::fromRows({2, 3, 4});
    REQUIRE(set.getNumRanges() == 1);
    CHECK(set.getRange(0).getStart() == 2);
    CHECK(set.getRange(0).getEnd() == 5);
}

TEST_CASE("scattered rows keep a range for each run", "[ui][row-selection]")
{
    const auto set = RowSelection::fromRows({1, 3, 4, 7});
    REQUIRE(set.getNumRanges() == 3);
    CHECK(set.getRange(0).getStart() == 1);
    CHECK(set.getRange(0).getEnd() == 2);
    CHECK(set.getRange(1).getStart() == 3);
    CHECK(set.getRange(1).getEnd() == 5);
    CHECK(set.getRange(2).getStart() == 7);
    CHECK(set.getRange(2).getEnd() == 8);
    CHECK(set.size() == 4);
    CHECK_FALSE(set.contains(2));
    CHECK_FALSE(set.contains(5));
}

TEST_CASE("a single row makes a one-row range", "[ui][row-selection]")
{
    const auto set = RowSelection::fromRows({0});
    REQUIRE(set.getNumRanges() == 1);
    CHECK(set.contains(0));
    CHECK(set.size() == 1);
}

TEST_CASE("every other row of thousands is kept", "[ui][row-selection]")
{
    std::vector<int> rows;
    for (int i = 0; i < 5000; ++i)
        rows.push_back(i * 2);
    const auto set = RowSelection::fromRows(rows);
    CHECK(set.getNumRanges() == 5000);
    CHECK(set.size() == 5000);
    CHECK(set.contains(9998));
    CHECK_FALSE(set.contains(9999));
}
