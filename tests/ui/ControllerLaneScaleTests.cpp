#include "ui/pianoroll/ControllerLaneScale.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("a pitch bend near the centre line snaps to the centre", "[ui][controller]")
{
    CHECK(ControllerLaneScale::pitchBendAt(3, 50) == 8192);
    CHECK(ControllerLaneScale::pitchBendAt(-3, 50) == 8192);
}

TEST_CASE("a pitch bend at the edges reaches the ends of its range", "[ui][controller]")
{
    CHECK(ControllerLaneScale::pitchBendAt(50, 50) == 16383);
    CHECK(ControllerLaneScale::pitchBendAt(-50, 50) == 0);
    CHECK(ControllerLaneScale::pitchBendAt(80, 50) == 16383);
}

TEST_CASE("a pitch bend away from the centre follows the offset", "[ui][controller]")
{
    CHECK(ControllerLaneScale::pitchBendAt(25, 50) == 12288);
}

TEST_CASE("a drag without vertical movement keeps the value", "[ui][controller]")
{
    const auto valueAtY = [](int y) { return 200 - y; };
    CHECK(ControllerLaneScale::draggedValue(100, 78, 0, valueAtY) == 100);
    CHECK(ControllerLaneScale::draggedValue(100, 78, -10, valueAtY) == 132);
}

TEST_CASE("a drag keeps the tick until the mouse moves sideways past the slop", "[ui][controller]")
{
    const auto snappedTickAtX = [](int x) { return x * 10; };
    CHECK(ControllerLaneScale::draggedTick(1234, 100, 2, snappedTickAtX) == 1234);
    CHECK(ControllerLaneScale::draggedTick(1234, 100, -2, snappedTickAtX) == 1234);
    CHECK(ControllerLaneScale::draggedTick(1234, 100, 3, snappedTickAtX) == 1030);
}

TEST_CASE("the nearest point within the radius is hit", "[ui][controller]")
{
    CHECK(ControllerLaneScale::nearestWithin({{-1.0f, 0.0f}, {4.0f, 0.0f}}, 6.0f) == 0);
    CHECK(ControllerLaneScale::nearestWithin({{7.0f, 0.0f}}, 6.0f) == -1);
}

TEST_CASE("of points at the same distance the later one is hit", "[ui][controller]")
{
    CHECK(ControllerLaneScale::nearestWithin({{0.0f, 2.0f}, {0.0f, -2.0f}}, 6.0f) == 1);
}
