#include "engine/transport/PlaybackClock.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
constexpr double ticksPerMsAt120 = 0.96;

PlaybackClock clockAt(double timeMs, double tick, int pass = 1)
{
    PlaybackClock clock;
    clock.timeMs = timeMs;
    clock.tick = tick;
    clock.ticksPerMs = ticksPerMsAt120;
    clock.pass = pass;
    return clock;
}
} // namespace

TEST_CASE("a later time advances by the tempo and truncates", "[engine][clock]")
{
    const auto clock = clockAt(1000.0, 480.0);
    CHECK(locate(clock, 1010.0) == ClockPosition{1, 489});
}

TEST_CASE("an earlier time within the pass goes back by the tempo", "[engine][clock]")
{
    const auto clock = clockAt(1000.0, 480.0);
    CHECK(locate(clock, 990.0) == ClockPosition{1, 470});
}

TEST_CASE("a stopped clock stays at its tick", "[engine][clock]")
{
    auto clock = clockAt(1000.0, 480.0);
    clock.ticksPerMs = 0.0;
    CHECK(locate(clock, 5000.0) == ClockPosition{1, 480});
}

TEST_CASE("crossing the loop end wraps into the next pass", "[engine][clock]")
{
    auto clock = clockAt(1000.0, 1910.0, 3);
    clock.loop = {true, 0, 1920};
    CHECK(locate(clock, 1020.0) == ClockPosition{4, 9});
}

TEST_CASE("overshooting more than a whole loop lands on the loop start like the engine", "[engine][clock]")
{
    PlaybackClock clock;
    clock.timeMs = 1000.0;
    clock.tick = 9.0;
    clock.ticksPerMs = 1.0;
    clock.pass = 1;
    clock.loop = {true, 0, 10};
    CHECK(locate(clock, 1030.0) == ClockPosition{2, 0});
}

TEST_CASE("without a loop the tick keeps going past the loop range", "[engine][clock]")
{
    auto clock = clockAt(1000.0, 1910.0, 3);
    clock.loop = {false, 0, 1920};
    CHECK(locate(clock, 1020.0) == ClockPosition{3, 1929});
}

TEST_CASE("a playhead already past the loop end does not wrap ahead of the engine", "[engine][clock]")
{
    auto clock = clockAt(1000.0, 2000.0, 3);
    clock.loop = {true, 0, 1920};
    CHECK(locate(clock, 1010.0) == ClockPosition{3, 2009});
}

TEST_CASE("input stamped before a wrap the clock has taken falls into the previous pass", "[engine][clock]")
{
    auto clock = clockAt(1000.0, 5.0, 4);
    clock.loop = {true, 0, 1920};
    clock.passStartTick = 0;
    clock.passStartedByLoop = true;
    CHECK(locate(clock, 990.0) == ClockPosition{3, 1915});
}

TEST_CASE("input stamped before a jump is placed at the jump target", "[engine][clock]")
{
    auto clock = clockAt(1000.0, 962.0, 2);
    clock.passStartTick = 960;
    CHECK(locate(clock, 990.0) == ClockPosition{2, 960});
}

TEST_CASE("input stamped before playback started is placed at the start", "[engine][clock]")
{
    auto clock = clockAt(1000.0, 0.0);
    CHECK(locate(clock, 900.0) == ClockPosition{1, 0});
}
