#include "ui/eventlist/EventListPosition.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
TimelineMap threeEightFromBarTwo()
{
    TimelineMap timeline;
    timeline.setTimeSignatureChanges({{0, 4, 4}, {1920, 3, 8}});
    return timeline;
}
} // namespace

TEST_CASE("a position is kept inside its bar and beat", "[ui][event-list]")
{
    const TimelineMap timeline;
    CHECK(EventListPosition::toTick({0, 1, 0}, timeline) == 0);
    CHECK(EventListPosition::toTick({1, 9, 0}, timeline) == 1440);
    CHECK(EventListPosition::toTick({1, 0, 0}, timeline) == 0);
    CHECK(EventListPosition::toTick({1, 1, 999}, timeline) == 479);
    CHECK(EventListPosition::toTick({2, 3, 240}, timeline) == 3120);
}

TEST_CASE("a position follows the time signature of its bar", "[ui][event-list]")
{
    const auto timeline = threeEightFromBarTwo();
    CHECK(EventListPosition::toTick({2, 4, 0}, timeline) == 2400);
    CHECK(EventListPosition::toTick({2, 1, 500}, timeline) == 2159);
    CHECK(EventListPosition::toTick({3, 1, 0}, timeline) == 2640);
}
