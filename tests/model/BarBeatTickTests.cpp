#include "support/BarBeatTickTestHelpers.h"
#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace timesigtest;

namespace
{
void setThreeSections(TimelineMap& timeline)
{
    setTimeSignatures(timeline, {{1, 4, 4}, {5, 3, 4}, {9, 6, 8}});
}
} // namespace

TEST_CASE("ticks map to bar, beat and tick in 4/4", "[timeline][bar-beat]")
{
    TimelineMap timeline;
    CHECK(timeline.tickToBarBeatTick(0) == BarBeatTick{1, 1, 0});
    CHECK(timeline.tickToBarBeatTick(479) == BarBeatTick{1, 1, 479});
    CHECK(timeline.tickToBarBeatTick(480) == BarBeatTick{1, 2, 0});
    CHECK(timeline.tickToBarBeatTick(1919) == BarBeatTick{1, 4, 479});
    CHECK(timeline.tickToBarBeatTick(1920) == BarBeatTick{2, 1, 0});
    CHECK(timeline.tickToBarBeatTick(4000) == BarBeatTick{3, 1, 160});
}

TEST_CASE("bar numbering continues across time signature changes", "[timeline][bar-beat]")
{
    TimelineMap timeline;
    setThreeSections(timeline);
    CHECK(timeline.tickToBarBeatTick(7679) == BarBeatTick{4, 4, 479});
    CHECK(timeline.tickToBarBeatTick(7680) == BarBeatTick{5, 1, 0});
    CHECK(timeline.tickToBarBeatTick(9120) == BarBeatTick{6, 1, 0});
    CHECK(timeline.tickToBarBeatTick(9700) == BarBeatTick{6, 2, 100});
    CHECK(timeline.tickToBarBeatTick(13439) == BarBeatTick{8, 3, 479});
    CHECK(timeline.tickToBarBeatTick(13440) == BarBeatTick{9, 1, 0});
    CHECK(timeline.tickToBarBeatTick(16090) == BarBeatTick{10, 6, 10});
}

TEST_CASE("bar starts follow the time signature map", "[timeline][bar-beat]")
{
    TimelineMap timeline;
    setThreeSections(timeline);
    CHECK(timeline.barStartToTick(1) == 0);
    CHECK(timeline.barStartToTick(5) == 7680);
    CHECK(timeline.barStartToTick(6) == 9120);
    CHECK(timeline.barStartToTick(8) == 12000);
    CHECK(timeline.barStartToTick(9) == 13440);
    CHECK(timeline.barStartToTick(12) == 17760);
}

TEST_CASE("bars before 1 start at tick 0", "[timeline][bar-beat]")
{
    TimelineMap timeline;
    setThreeSections(timeline);
    CHECK(timeline.barStartToTick(0) == 0);
    CHECK(timeline.barStartToTick(-3) == 0);
}

TEST_CASE("bar, beat and tick map back to ticks", "[timeline][bar-beat]")
{
    TimelineMap timeline;
    setThreeSections(timeline);
    CHECK(timeline.barBeatTickToTick(1, 1, 0) == 0);
    CHECK(timeline.barBeatTickToTick(6, 2, 100) == 9700);
    CHECK(timeline.barBeatTickToTick(10, 6, 10) == 16090);
}

TEST_CASE("out-of-range bar positions are clamped", "[timeline][bar-beat]")
{
    TimelineMap timeline;
    setThreeSections(timeline);
    CHECK(timeline.barBeatTickToTick(0, 1, 0) == 0);
    CHECK(timeline.barBeatTickToTick(-2, 2, 0) == 480);
    CHECK(timeline.barBeatTickToTick(1, 1, -50) == 0);
}

TEST_CASE("tick to bar position round-trips", "[timeline][bar-beat]")
{
    TimelineMap timeline;
    setThreeSections(timeline);
    for (int t : {0, 479, 1920, 7679, 7680, 9700, 13439, 13440, 16090, 30000})
    {
        CAPTURE(t);
        const auto p = timeline.tickToBarBeatTick(t);
        CHECK(timeline.barBeatTickToTick(p.bar, p.beat, p.tick) == t);
    }
}

TEST_CASE("bar starts round-trip", "[timeline][bar-beat]")
{
    TimelineMap timeline;
    setThreeSections(timeline);
    for (int n : {1, 2, 4, 5, 6, 8, 9, 10, 15})
    {
        CAPTURE(n);
        CHECK(timeline.tickToBarBeatTick(timeline.barStartToTick(n)) == BarBeatTick{n, 1, 0});
    }
}
