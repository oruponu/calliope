#include "support/BarBeatTickTestHelpers.h"
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

BarBeatTick stepped(BarBeatTick position, PositionField field, int direction,
                    const TimelineMap& timeline = TimelineMap{})
{
    return EventListPosition::stepped(position, field, direction, timeline);
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

TEST_CASE("a tick step carries into the beats and bars", "[ui][event-list]")
{
    CHECK(stepped({1, 1, 479}, PositionField::Tick, 1) == BarBeatTick{1, 2, 0});
    CHECK(stepped({1, 4, 479}, PositionField::Tick, 1) == BarBeatTick{2, 1, 0});
    CHECK(stepped({2, 1, 0}, PositionField::Tick, -1) == BarBeatTick{1, 4, 479});
    CHECK(stepped({1, 1, 10}, PositionField::Tick, -1) == BarBeatTick{1, 1, 9});
}

TEST_CASE("a beat step carries into the bars and keeps the tick", "[ui][event-list]")
{
    CHECK(stepped({1, 2, 100}, PositionField::Beat, 1) == BarBeatTick{1, 3, 100});
    CHECK(stepped({1, 4, 100}, PositionField::Beat, 1) == BarBeatTick{2, 1, 100});
    CHECK(stepped({2, 1, 100}, PositionField::Beat, -1) == BarBeatTick{1, 4, 100});
}

TEST_CASE("a bar step keeps the beat and tick", "[ui][event-list]")
{
    CHECK(stepped({1, 3, 100}, PositionField::Bar, 1) == BarBeatTick{2, 3, 100});
    CHECK(stepped({3, 3, 100}, PositionField::Bar, -1) == BarBeatTick{2, 3, 100});
}

TEST_CASE("a step does not go before the first tick", "[ui][event-list]")
{
    CHECK(stepped({1, 1, 0}, PositionField::Tick, -1) == BarBeatTick{1, 1, 0});
    CHECK(stepped({1, 1, 100}, PositionField::Beat, -1) == BarBeatTick{1, 1, 100});
    CHECK(stepped({1, 3, 100}, PositionField::Bar, -1) == BarBeatTick{1, 3, 100});
}

TEST_CASE("a step into a bar of another time signature keeps the beat and tick inside it", "[ui][event-list]")
{
    const auto timeline = threeEightFromBarTwo();
    CHECK(stepped({1, 4, 300}, PositionField::Beat, 1, timeline) == BarBeatTick{2, 1, 239});
    CHECK(stepped({1, 4, 300}, PositionField::Bar, 1, timeline) == BarBeatTick{2, 3, 239});
    CHECK(stepped({2, 3, 0}, PositionField::Beat, 1, timeline) == BarBeatTick{3, 1, 0});
    CHECK(stepped({2, 1, 0}, PositionField::Beat, -1, timeline) == BarBeatTick{1, 4, 0});
    CHECK(stepped({2, 3, 239}, PositionField::Tick, 1, timeline) == BarBeatTick{3, 1, 0});
}

TEST_CASE("a position typed out of range is kept inside its bar before it steps", "[ui][event-list]")
{
    CHECK(stepped({1, 9, 999}, PositionField::Tick, 1) == BarBeatTick{2, 1, 0});
    CHECK(stepped({0, 1, 0}, PositionField::Beat, 1) == BarBeatTick{1, 2, 0});
}
