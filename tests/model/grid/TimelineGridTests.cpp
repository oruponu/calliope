#include "model/grid/TimelineGrid.h"
#include "support/GridLineStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace
{
using enum GridLineKind;
using Lines = std::vector<GridLine>;

constexpr int lastTick = std::numeric_limits<int>::max();
constexpr int firstTick = std::numeric_limits<int>::min();

TimelineMap timelineWith(std::vector<TimeSignatureChange> signatures, int ppq = 480)
{
    TimelineMap timeline;
    timeline.setTicksPerQuarterNote(ppq);
    timeline.setTimeSignatureChanges(std::move(signatures));
    return timeline;
}
} // namespace

TEST_CASE("a straight grid in 4/4 rounds like counting from tick 0", "[model][grid]")
{
    const TimelineMap timeline;
    const TimelineGrid quarters(timeline, {4});
    CHECK(quarters.round(0) == 0);
    CHECK(quarters.round(239) == 0);
    CHECK(quarters.round(240) == 480);
    CHECK(quarters.round(719) == 480);
    CHECK(quarters.round(720) == 960);

    const TimelineGrid sixteenths(timeline, {16});
    CHECK(sixteenths.round(59) == 0);
    CHECK(sixteenths.round(60) == 120);
    CHECK(sixteenths.round(1979) == 1920);
    CHECK(sixteenths.round(1980) == 2040);
}

TEST_CASE("ticks floor to the line at or before them", "[model][grid]")
{
    const TimelineMap timeline;
    const TimelineGrid grid(timeline, {4});
    CHECK(grid.floor(0) == 0);
    CHECK(grid.floor(479) == 0);
    CHECK(grid.floor(480) == 480);
    CHECK(grid.floor(959) == 480);
}

TEST_CASE("a half-note grid in 3/4 ends each bar with a short step", "[model][grid]")
{
    const auto timeline = timelineWith({{0, 3, 4}});
    const TimelineGrid grid(timeline, {2});
    CHECK(grid.lines(0, 2880) ==
          Lines{{0, Bar}, {480, Beat}, {960, Beat}, {1440, Bar}, {1920, Beat}, {2400, Beat}, {2880, Bar}});
    CHECK(grid.round(1199) == 960);
    CHECK(grid.round(1200) == 1440);
    CHECK(grid.round(1900) == 1440);
    CHECK(grid.round(1920) == 2400);
    CHECK(grid.floor(2399) == 1440);
}

TEST_CASE("next and previous step over bar lines", "[model][grid]")
{
    const auto timeline = timelineWith({{0, 3, 4}});
    const TimelineGrid grid(timeline, {2});
    CHECK(grid.next(960) == 1440);
    CHECK(grid.next(1439) == 1440);
    CHECK(grid.next(1440) == 2400);
    CHECK(grid.previous(1440) == 960);
    CHECK(grid.previous(961) == 960);
    CHECK(grid.previous(960) == 0);
}

TEST_CASE("a time signature change starts counting again", "[model][grid]")
{
    const auto timeline = timelineWith({{0, 3, 8}, {720, 4, 4}});
    const TimelineGrid grid(timeline, {4});
    CHECK(grid.lines(0, 1200) == Lines{{0, Bar}, {240, Beat}, {480, Beat}, {720, Bar}, {1200, Beat}});
    CHECK(grid.round(900) == 720);
    CHECK(grid.round(1000) == 1200);
}

TEST_CASE("a bar cut short by the next time signature ends on its start", "[model][grid]")
{
    const auto timeline = timelineWith({{0, 4, 4}, {2400, 3, 4}});
    const TimelineGrid grid(timeline, {4});
    CHECK(grid.lines(1920, 2880) == Lines{{1920, Bar}, {2400, Bar}, {2880, Beat}});
    CHECK(grid.floor(2399) == 1920);
    CHECK(grid.round(2300) == 2400);
}

TEST_CASE("the later of two time signatures on one tick wins", "[model][grid]")
{
    const auto timeline = timelineWith({{0, 4, 4}, {1920, 3, 4}, {1920, 6, 8}});
    const TimelineGrid grid(timeline, {4});
    CHECK(grid.lines(1920, 3360) ==
          Lines{{1920, Bar}, {2160, Beat}, {2400, Beat}, {2640, Beat}, {2880, Beat}, {3120, Beat}, {3360, Bar}});
}

TEST_CASE("without time signatures the grid follows 4/4", "[model][grid]")
{
    const auto timeline = timelineWith({});
    const TimelineGrid grid(timeline, {4});
    CHECK(grid.lines(0, 1920) == Lines{{0, Bar}, {480, Beat}, {960, Beat}, {1440, Beat}, {1920, Bar}});
}

TEST_CASE("a step that is not a whole tick is floored within each bar", "[model][grid]")
{
    const auto timeline = timelineWith({{0, 4, 4}}, 100);
    const TimelineGrid grid(timeline, {32});
    CHECK(grid.next(0) == 12);
    CHECK(grid.next(12) == 25);
    CHECK(grid.next(25) == 37);
    CHECK(grid.floor(36) == 25);
    CHECK(grid.floor(37) == 37);
    CHECK(grid.floor(30011) == 30000);
    CHECK(grid.floor(30012) == 30012);
}

TEST_CASE("lines cover only the asked range and never go before tick 0", "[model][grid]")
{
    const TimelineMap timeline;
    const TimelineGrid grid(timeline, {4});
    CHECK(grid.lines(500, 1000) == Lines{{960, Beat}});
    CHECK(grid.lines(-500, 0) == Lines{{0, Bar}});
    CHECK(grid.lines(10, 5).empty());
}

TEST_CASE("near the last tick round keeps the earlier line and next has none", "[model][grid]")
{
    const TimelineMap timeline;
    const TimelineGrid grid(timeline, {16});
    CHECK(grid.round(lastTick) == lastTick - 7);
    CHECK(grid.round(lastTick - 37) == lastTick - 7);
    CHECK(grid.next(lastTick - 8) == lastTick - 7);
    CHECK(grid.next(lastTick - 7) == std::nullopt);
}

TEST_CASE("ticks before 0 continue the first time signature backwards", "[model][grid]")
{
    const TimelineMap timeline;
    const TimelineGrid grid(timeline, {4});
    CHECK(grid.previous(0) == -480);
    CHECK(grid.floor(-1) == -480);
    CHECK(grid.round(-300) == -480);
    CHECK(grid.round(-200) == 0);
}

TEST_CASE("at the lowest tick floor and round take the first line that fits and previous has none", "[model][grid]")
{
    const TimelineMap timeline;
    const TimelineGrid grid(timeline, {4});
    CHECK(grid.floor(firstTick) == firstTick + 128);
    CHECK(grid.round(firstTick) == firstTick + 128);
    CHECK(grid.previous(firstTick) == std::nullopt);
}

TEST_CASE("a step finer than one tick makes every tick a line", "[model][grid]")
{
    const auto timeline = timelineWith({{0, 4, 4}}, 1);
    const TimelineGrid grid(timeline, {32});
    CHECK(grid.round(3) == 3);
    CHECK(grid.next(3) == 4);
    CHECK(grid.lines(0, 4) == Lines{{0, Bar}, {1, Beat}, {2, Beat}, {3, Beat}, {4, Bar}});
    CHECK(grid.stepTicks() == 1);
}

TEST_CASE("the step in ticks follows the resolution", "[model][grid]")
{
    const TimelineMap timeline;
    CHECK(TimelineGrid(timeline, {4}).stepTicks() == 480);

    const auto coarse = timelineWith({{0, 4, 4}}, 100);
    CHECK(TimelineGrid(coarse, {32}).stepTicks() == 13);
}

TEST_CASE("eighth-note triplets fall every 160 ticks in 4/4", "[model][grid]")
{
    const TimelineMap timeline;
    const TimelineGrid grid(timeline, {8, true});
    CHECK(grid.lines(0, 960) ==
          Lines{{0, Bar}, {160, Sub}, {320, Sub}, {480, Beat}, {640, Sub}, {800, Sub}, {960, Beat}});
}

TEST_CASE("quarter-note triplets in 4/4 keep the beat lines but never snap to them", "[model][grid]")
{
    const TimelineMap timeline;
    const TimelineGrid grid(timeline, {4, true});
    CHECK(grid.lines(0, 1920) == Lines{{0, Bar},
                                       {320, Sub},
                                       {480, Beat},
                                       {640, Sub},
                                       {960, Beat},
                                       {1280, Sub},
                                       {1440, Beat},
                                       {1600, Sub},
                                       {1920, Bar}});
    CHECK(grid.round(480) == 640);
    CHECK(grid.floor(480) == 320);
}

TEST_CASE("quarter-note triplets in 3/4 end each bar with a short step", "[model][grid]")
{
    const auto timeline = timelineWith({{0, 3, 4}});
    const TimelineGrid grid(timeline, {4, true});
    CHECK(grid.lines(0, 1440) ==
          Lines{{0, Bar}, {320, Sub}, {480, Beat}, {640, Sub}, {960, Beat}, {1280, Sub}, {1440, Bar}});
    CHECK(grid.lines(1440, 2880) ==
          Lines{{1440, Bar}, {1760, Sub}, {1920, Beat}, {2080, Sub}, {2400, Beat}, {2720, Sub}, {2880, Bar}});
    CHECK(grid.round(1359) == 1280);
    CHECK(grid.round(1360) == 1440);
    CHECK(grid.next(1280) == 1440);
    CHECK(grid.previous(1440) == 1280);
}

TEST_CASE("triplet lines with a fractional step stay aligned in later bars", "[model][grid]")
{
    const auto timeline = timelineWith({{0, 3, 4}}, 100);
    const TimelineGrid grid(timeline, {2, true});
    CHECK(grid.floor(299) == 266);
    CHECK(grid.next(266) == 300);
    CHECK(grid.round(282) == 266);
    CHECK(grid.round(283) == 300);
    CHECK(grid.lines(300, 600) == Lines{{300, Bar}, {400, Beat}, {433, Sub}, {500, Beat}, {566, Sub}, {600, Bar}});
    CHECK(grid.floor(30132) == 30000);
    CHECK(grid.floor(30133) == 30133);
}

TEST_CASE("the every-tick grid keeps every tick where it is", "[model][grid]")
{
    const auto timeline = timelineWith({{0, 3, 4}});
    const auto grid = TimelineGrid::everyTick(timeline);
    CHECK(grid.round(0) == 0);
    CHECK(grid.round(7) == 7);
    CHECK(grid.round(1439) == 1439);
    CHECK(grid.round(-5) == -5);
    CHECK(grid.floor(7) == 7);
    CHECK(grid.floor(-5) == -5);
    CHECK(grid.stepTicks() == 1);
}

TEST_CASE("the every-tick grid steps one tick and has no line outside int", "[model][grid]")
{
    const TimelineMap timeline;
    const auto grid = TimelineGrid::everyTick(timeline);
    CHECK(grid.next(7) == 8);
    CHECK(grid.previous(7) == 6);
    CHECK(grid.next(1919) == 1920);
    CHECK(grid.previous(1920) == 1919);
    CHECK(grid.next(lastTick) == std::nullopt);
    CHECK(grid.previous(firstTick) == std::nullopt);
}
