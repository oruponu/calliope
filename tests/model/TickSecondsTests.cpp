#include "model/MidiSequence.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("at a single tempo ticks convert at a constant rate", "[timeline][seconds]")
{
    MidiSequence seq;
    CHECK_THAT(seq.ticksToSeconds(0), WithinAbs(0.0, 1e-9));
    CHECK_THAT(seq.ticksToSeconds(480), WithinAbs(0.5, 1e-9));
    CHECK_THAT(seq.ticksToSeconds(960), WithinAbs(1.0, 1e-9));
    CHECK(seq.secondsToTicks(1.0) == 960);
    CHECK(seq.secondsToTicks(2.5) == 2400);
}

TEST_CASE("time accumulates across tempo changes", "[timeline][seconds]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 120.0}, {1920, 60.0}});
    CHECK_THAT(seq.ticksToSeconds(1920), WithinAbs(2.0, 1e-9));
    CHECK_THAT(seq.ticksToSeconds(2400), WithinAbs(3.0, 1e-9));
    CHECK(seq.secondsToTicks(1.5) == 1440);
    CHECK(seq.secondsToTicks(2.0) == 1920);
    CHECK(seq.secondsToTicks(3.0) == 2400);
}

TEST_CASE("seconds before the first tempo change use the default tempo", "[timeline][seconds]")
{
    MidiSequence seq;
    seq.setTempoChanges({{960, 60.0}});
    CHECK_THAT(seq.ticksToSeconds(960), WithinAbs(1.0, 1e-9));
    CHECK_THAT(seq.ticksToSeconds(1440), WithinAbs(2.0, 1e-9));
    CHECK(seq.secondsToTicks(1.0) == 960);
    CHECK(seq.secondsToTicks(2.0) == 1440);
}

TEST_CASE("ticks round-trip through seconds", "[timeline][seconds]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 120.0}, {1920, 60.0}, {5000, 100.0}});
    for (int t : {0, 1, 479, 1919, 1920, 1921, 4999, 5000, 5001, 7777, 100003})
    {
        CAPTURE(t);
        CHECK(seq.secondsToTicks(seq.ticksToSeconds(t)) == t);
    }
}
