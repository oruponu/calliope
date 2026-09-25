#include "support/BarBeatTickTestHelpers.h"
#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace timesigtest;

namespace
{
void setThreeSections(MidiSequence& seq)
{
    setTimeSignatures(seq, {{1, 4, 4}, {5, 3, 4}, {9, 6, 8}});
}
} // namespace

TEST_CASE("ticks map to bar, beat and tick in 4/4", "[timeline][bar-beat]")
{
    MidiSequence seq;
    CHECK(seq.tickToBarBeatTick(0) == BarBeatTick{1, 1, 0});
    CHECK(seq.tickToBarBeatTick(479) == BarBeatTick{1, 1, 479});
    CHECK(seq.tickToBarBeatTick(480) == BarBeatTick{1, 2, 0});
    CHECK(seq.tickToBarBeatTick(1919) == BarBeatTick{1, 4, 479});
    CHECK(seq.tickToBarBeatTick(1920) == BarBeatTick{2, 1, 0});
    CHECK(seq.tickToBarBeatTick(4000) == BarBeatTick{3, 1, 160});
}

TEST_CASE("bar numbering continues across time signature changes", "[timeline][bar-beat]")
{
    MidiSequence seq;
    setThreeSections(seq);
    CHECK(seq.tickToBarBeatTick(7679) == BarBeatTick{4, 4, 479});
    CHECK(seq.tickToBarBeatTick(7680) == BarBeatTick{5, 1, 0});
    CHECK(seq.tickToBarBeatTick(9120) == BarBeatTick{6, 1, 0});
    CHECK(seq.tickToBarBeatTick(9700) == BarBeatTick{6, 2, 100});
    CHECK(seq.tickToBarBeatTick(13439) == BarBeatTick{8, 3, 479});
    CHECK(seq.tickToBarBeatTick(13440) == BarBeatTick{9, 1, 0});
    CHECK(seq.tickToBarBeatTick(16090) == BarBeatTick{10, 6, 10});
}

TEST_CASE("bar starts follow the time signature map", "[timeline][bar-beat]")
{
    MidiSequence seq;
    setThreeSections(seq);
    CHECK(seq.barStartToTick(1) == 0);
    CHECK(seq.barStartToTick(5) == 7680);
    CHECK(seq.barStartToTick(6) == 9120);
    CHECK(seq.barStartToTick(8) == 12000);
    CHECK(seq.barStartToTick(9) == 13440);
    CHECK(seq.barStartToTick(12) == 17760);
}

TEST_CASE("bars before 1 start at tick 0", "[timeline][bar-beat]")
{
    MidiSequence seq;
    setThreeSections(seq);
    CHECK(seq.barStartToTick(0) == 0);
    CHECK(seq.barStartToTick(-3) == 0);
}

TEST_CASE("bar, beat and tick map back to ticks", "[timeline][bar-beat]")
{
    MidiSequence seq;
    setThreeSections(seq);
    CHECK(seq.barBeatTickToTick(1, 1, 0) == 0);
    CHECK(seq.barBeatTickToTick(6, 2, 100) == 9700);
    CHECK(seq.barBeatTickToTick(10, 6, 10) == 16090);
}

TEST_CASE("out-of-range bar positions are clamped", "[timeline][bar-beat]")
{
    MidiSequence seq;
    setThreeSections(seq);
    CHECK(seq.barBeatTickToTick(0, 1, 0) == 0);
    CHECK(seq.barBeatTickToTick(-2, 2, 0) == 480);
    CHECK(seq.barBeatTickToTick(1, 1, -50) == 0);
}

TEST_CASE("tick to bar position round-trips", "[timeline][bar-beat]")
{
    MidiSequence seq;
    setThreeSections(seq);
    for (int t : {0, 479, 1920, 7679, 7680, 9700, 13439, 13440, 16090, 30000})
    {
        CAPTURE(t);
        const auto p = seq.tickToBarBeatTick(t);
        CHECK(seq.barBeatTickToTick(p.bar, p.beat, p.tick) == t);
    }
}

TEST_CASE("bar starts round-trip", "[timeline][bar-beat]")
{
    MidiSequence seq;
    setThreeSections(seq);
    for (int n : {1, 2, 4, 5, 6, 8, 9, 10, 15})
    {
        CAPTURE(n);
        CHECK(seq.tickToBarBeatTick(seq.barStartToTick(n)) == BarBeatTick{n, 1, 0});
    }
}
