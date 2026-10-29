#include "engine/metronome/CountIn.h"
#include "support/MetronomeClickStringMaker.h"
#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using TimeSignatureTest::ppq;
using TimeSignatureTest::timeSigs;

TEST_CASE("a count-in to a bar start in 4/4 clicks four beats with the first accented", "[engine][count-in]")
{
    const auto countIn = makeCountIn(timeSigs({{1, 4, 4}}), ppq, 3840, 1);
    CHECK(countIn.lengthTicks == 1920);
    CHECK(countInClicksInRange(countIn, 0, 1920) ==
          std::vector<MetronomeClick>{{0, true}, {480, false}, {960, false}, {1440, false}});
}

TEST_CASE("a count-in in 6/8 clicks six eighths with the first accented", "[engine][count-in]")
{
    const auto countIn = makeCountIn(timeSigs({{1, 6, 8}}), ppq, 1440, 1);
    CHECK(countIn.lengthTicks == 1440);
    CHECK(
        countInClicksInRange(countIn, 0, 1440) ==
        std::vector<MetronomeClick>{{0, true}, {240, false}, {480, false}, {720, false}, {960, false}, {1200, false}});
}

TEST_CASE("a count-in of two bars accents each bar start", "[engine][count-in]")
{
    const auto countIn = makeCountIn(timeSigs({{1, 4, 4}}), ppq, 3840, 2);
    CHECK(countIn.lengthTicks == 3840);
    CHECK(countInClicksInRange(countIn, 0, 3840) == std::vector<MetronomeClick>{{0, true},
                                                                                {480, false},
                                                                                {960, false},
                                                                                {1440, false},
                                                                                {1920, true},
                                                                                {2400, false},
                                                                                {2880, false},
                                                                                {3360, false}});
}

TEST_CASE("a count-in to the middle of a bar lasts one bar and accents the bar start inside it", "[engine][count-in]")
{
    const auto countIn = makeCountIn(timeSigs({{1, 4, 4}}), ppq, 8640, 1);
    CHECK(countIn.lengthTicks == 1920);
    CHECK(countInClicksInRange(countIn, 0, 1920) ==
          std::vector<MetronomeClick>{{0, false}, {480, false}, {960, true}, {1440, false}});
}

TEST_CASE("a count-in to the song start without time signatures counts a bar of 4/4", "[engine][count-in]")
{
    const auto countIn = makeCountIn({}, ppq, 0, 1);
    CHECK(countIn.lengthTicks == 1920);
    CHECK(countInClicksInRange(countIn, 0, 1920) ==
          std::vector<MetronomeClick>{{0, true}, {480, false}, {960, false}, {1440, false}});
}

TEST_CASE("a count-in follows the time signature where it ends, not the one before", "[engine][count-in]")
{
    const auto countIn = makeCountIn(timeSigs({{1, 3, 4}, {2, 4, 4}}), ppq, 1440, 1);
    CHECK(countIn.lengthTicks == 1920);
    CHECK(countInClicksInRange(countIn, 0, 1920) ==
          std::vector<MetronomeClick>{{0, true}, {480, false}, {960, false}, {1440, false}});
}

TEST_CASE("a count-in to an off-beat start clicks on the beats", "[engine][count-in]")
{
    const auto countIn = makeCountIn(timeSigs({{1, 4, 4}}), ppq, 2020, 1);
    CHECK(countIn.lengthTicks == 1920);
    CHECK(countInClicksInRange(countIn, 0, 1920) ==
          std::vector<MetronomeClick>{{380, false}, {860, false}, {1340, false}, {1820, true}});
}

TEST_CASE("count-in clicks include fromTick and exclude toTick", "[engine][count-in]")
{
    const auto countIn = makeCountIn(timeSigs({{1, 4, 4}}), ppq, 3840, 1);
    CHECK(countInClicksInRange(countIn, 480, 960) == std::vector<MetronomeClick>{{480, false}});
    CHECK(countInClicksInRange(countIn, 481, 960).empty());
    CHECK(countInClicksInRange(countIn, 480, 480).empty());
}

TEST_CASE("count-in clicks stop at the end of the count-in", "[engine][count-in]")
{
    const auto countIn = makeCountIn(timeSigs({{1, 4, 4}}), ppq, 3840, 1);
    CHECK(countInClicksInRange(countIn, 0, 5000) ==
          std::vector<MetronomeClick>{{0, true}, {480, false}, {960, false}, {1440, false}});
}

TEST_CASE("a count-in scales with the ticks per quarter note", "[engine][count-in]")
{
    const auto countIn = makeCountIn({{0, 4, 4}}, 96, 384, 1);
    CHECK(countIn.lengthTicks == 384);
    CHECK(countInClicksInRange(countIn, 0, 384) ==
          std::vector<MetronomeClick>{{0, true}, {96, false}, {192, false}, {288, false}});
}

TEST_CASE("the bar start before a tick in the middle of a bar is the start of that bar", "[engine][count-in]")
{
    CHECK(barStartAtOrBefore(timeSigs({{1, 4, 4}}), ppq, 2020) == 1920);
}

TEST_CASE("the bar start at a bar start is that tick", "[engine][count-in]")
{
    CHECK(barStartAtOrBefore(timeSigs({{1, 4, 4}}), ppq, 3840) == 3840);
}

TEST_CASE("the bar start after a time signature change counts from the change", "[engine][count-in]")
{
    CHECK(barStartAtOrBefore(timeSigs({{1, 3, 4}, {2, 4, 4}}), ppq, 3860) == 3360);
}

TEST_CASE("the bar start without time signatures counts bars of 4/4 from tick 0", "[engine][count-in]")
{
    CHECK(barStartAtOrBefore({}, ppq, 0) == 0);
    CHECK(barStartAtOrBefore({}, ppq, 2000) == 1920);
}

TEST_CASE("a count-in to the bar start before the playhead accents its first click", "[engine][count-in]")
{
    const auto signatures = timeSigs({{1, 6, 8}});
    const auto countIn = makeCountIn(signatures, ppq, barStartAtOrBefore(signatures, ppq, 3500), 1);
    CHECK(
        countInClicksInRange(countIn, 0, 1440) ==
        std::vector<MetronomeClick>{{0, true}, {240, false}, {480, false}, {720, false}, {960, false}, {1200, false}});
}
