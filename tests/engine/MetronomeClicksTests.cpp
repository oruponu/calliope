#include "engine/MetronomeClicks.h"
#include "model/TimelineMap.h"
#include "support/MetronomeClickStringMaker.h"
#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <utility>
#include <vector>

using timesigtest::ppq;
using timesigtest::timeSigs;

TEST_CASE("4/4 bar yields one accented click and three plain clicks", "[engine][metronome]")
{
    const auto clicks = metronomeClicksInRange(timeSigs({{1, 4, 4}}), ppq, 0, 1920);
    CHECK(clicks == std::vector<MetronomeClick>{{0, true}, {480, false}, {960, false}, {1440, false}});
}

TEST_CASE("6/8 bar clicks on every eighth with only the first accented", "[engine][metronome]")
{
    const auto clicks = metronomeClicksInRange(timeSigs({{1, 6, 8}}), ppq, 0, 1440);
    CHECK(clicks == std::vector<MetronomeClick>{
                        {0, true}, {240, false}, {480, false}, {720, false}, {960, false}, {1200, false}});
}

TEST_CASE("a beat at fromTick is included and a beat at toTick is excluded", "[engine][metronome]")
{
    const auto clicks = metronomeClicksInRange(timeSigs({{1, 4, 4}}), ppq, 480, 960);
    CHECK(clicks == std::vector<MetronomeClick>{{480, false}});
}

TEST_CASE("empty or reversed range yields no clicks", "[engine][metronome]")
{
    CHECK(metronomeClicksInRange(timeSigs({{1, 4, 4}}), ppq, 480, 480).empty());
    CHECK(metronomeClicksInRange(timeSigs({{1, 4, 4}}), ppq, 960, 480).empty());
}

TEST_CASE("range starting mid-beat begins at the next beat", "[engine][metronome]")
{
    const auto clicks = metronomeClicksInRange(timeSigs({{1, 4, 4}}), ppq, 100, 1000);
    CHECK(clicks == std::vector<MetronomeClick>{{480, false}, {960, false}});
}

TEST_CASE("range between two beats yields no clicks", "[engine][metronome]")
{
    CHECK(metronomeClicksInRange(timeSigs({{1, 4, 4}}), ppq, 10, 400).empty());
}

TEST_CASE("time signature change switches beat length and accents at the change", "[engine][metronome]")
{
    const auto clicks = metronomeClicksInRange(timeSigs({{1, 4, 4}, {2, 3, 4}}), ppq, 1440, 3840);
    CHECK(clicks ==
          std::vector<MetronomeClick>{{1440, false}, {1920, true}, {2400, false}, {2880, false}, {3360, true}});
}

TEST_CASE("non-default ppq scales the beat length", "[engine][metronome]")
{
    const auto clicks = metronomeClicksInRange({{0, 4, 4}}, 96, 0, 384);
    CHECK(clicks == std::vector<MetronomeClick>{{0, true}, {96, false}, {192, false}, {288, false}});
}

TEST_CASE("half-note and sixteenth-note denominators set the beat length", "[engine][metronome]")
{
    CHECK(metronomeClicksInRange({{0, 2, 2}}, ppq, 0, 1920) == std::vector<MetronomeClick>{{0, true}, {960, false}});
    CHECK(metronomeClicksInRange({{0, 3, 16}}, ppq, 0, 360) ==
          std::vector<MetronomeClick>{{0, true}, {120, false}, {240, false}});
}

TEST_CASE("empty time signature list falls back to 4/4", "[engine][metronome]")
{
    const auto clicks = metronomeClicksInRange({}, ppq, 0, 1920);
    CHECK(clicks == std::vector<MetronomeClick>{{0, true}, {480, false}, {960, false}, {1440, false}});
}

TEST_CASE("contiguous ranges neither drop nor duplicate clicks", "[engine][metronome]")
{
    const auto sigs = timeSigs({{1, 4, 4}, {2, 3, 4}});
    const auto whole = metronomeClicksInRange(sigs, ppq, 0, 3840);

    std::vector<MetronomeClick> pieces;
    for (const auto [from, to] :
         {std::pair{0, 700}, std::pair{700, 1920}, std::pair{1920, 1921}, std::pair{1921, 3360}, std::pair{3360, 3840}})
    {
        const auto clicks = metronomeClicksInRange(sigs, ppq, from, to);
        pieces.insert(pieces.end(), clicks.begin(), clicks.end());
    }

    CHECK(pieces == whole);
}

TEST_CASE("every click lands on a beat head counted by TimelineMap", "[engine][metronome]")
{
    TimelineMap timeline;
    timesigtest::setTimeSignatures(timeline, {{1, 4, 4}, {3, 6, 8}, {5, 5, 4}});

    const auto clicks =
        metronomeClicksInRange(timeline.getTimeSignatureChanges(), timeline.getTicksPerQuarterNote(), 0, 20000);

    CHECK(clicks.size() == 48);
    for (const auto& click : clicks)
    {
        const auto position = timeline.tickToBarBeatTick(click.tick);
        CHECK(position.tick == 0);
        CHECK(click.accent == (position.beat == 1));
    }
}

TEST_CASE("long range returns every beat", "[engine][metronome]")
{
    const auto clicks = metronomeClicksInRange(timeSigs({{1, 4, 4}}), ppq, 0, 192000);
    CHECK(clicks.size() == 400);
    CHECK(clicks.back() == MetronomeClick{191520, false});
}
