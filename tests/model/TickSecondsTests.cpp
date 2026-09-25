#include "model/TimelineMap.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("at a single tempo ticks convert at a constant rate", "[timeline][seconds]")
{
    TimelineMap timeline;
    CHECK_THAT(timeline.ticksToSeconds(0), WithinAbs(0.0, 1e-9));
    CHECK_THAT(timeline.ticksToSeconds(480), WithinAbs(0.5, 1e-9));
    CHECK_THAT(timeline.ticksToSeconds(960), WithinAbs(1.0, 1e-9));
    CHECK(timeline.secondsToTicks(1.0) == 960);
    CHECK(timeline.secondsToTicks(2.5) == 2400);
}

TEST_CASE("time accumulates across tempo changes", "[timeline][seconds]")
{
    TimelineMap timeline;
    timeline.setTempoChanges({{0, 120.0}, {1920, 60.0}});
    CHECK_THAT(timeline.ticksToSeconds(1920), WithinAbs(2.0, 1e-9));
    CHECK_THAT(timeline.ticksToSeconds(2400), WithinAbs(3.0, 1e-9));
    CHECK(timeline.secondsToTicks(1.5) == 1440);
    CHECK(timeline.secondsToTicks(2.0) == 1920);
    CHECK(timeline.secondsToTicks(3.0) == 2400);
}

TEST_CASE("seconds before the first tempo change use the default tempo", "[timeline][seconds]")
{
    TimelineMap timeline;
    timeline.setTempoChanges({{960, 60.0}});
    CHECK_THAT(timeline.ticksToSeconds(960), WithinAbs(1.0, 1e-9));
    CHECK_THAT(timeline.ticksToSeconds(1440), WithinAbs(2.0, 1e-9));
    CHECK(timeline.secondsToTicks(1.0) == 960);
    CHECK(timeline.secondsToTicks(2.0) == 1440);
}

TEST_CASE("ticks round-trip through seconds", "[timeline][seconds]")
{
    TimelineMap timeline;
    timeline.setTempoChanges({{0, 120.0}, {1920, 60.0}, {5000, 100.0}});
    for (int t : {0, 1, 479, 1919, 1920, 1921, 4999, 5000, 5001, 7777, 100003})
    {
        CAPTURE(t);
        CHECK(timeline.secondsToTicks(timeline.ticksToSeconds(t)) == t);
    }
}
