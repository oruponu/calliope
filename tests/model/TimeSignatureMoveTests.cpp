#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace timesigtest;

namespace
{
std::vector<TimeSignatureChange> threeSections()
{
    return timeSigs({{1, 4, 4}, {5, 3, 4}, {9, 6, 8}});
}

std::vector<TimeSignatureChange> moveTo(const std::vector<TimeSignatureChange>& before, const std::vector<int>& moved,
                                        int anchor, int targetTick)
{
    return MidiSequence::buildTimeSignatureChangesAfterMove(before, moved, anchor, targetTick, ppq);
}
} // namespace

TEST_CASE("moving a change keeps the bar numbers of the following changes", "[timesig][move]")
{
    CHECK(moveTo(threeSections(), {1}, 1, 3840) ==
          std::vector<TimeSignatureChange>{{0, 4, 4}, {3840, 3, 4}, {12480, 6, 8}});
}

TEST_CASE("a moved time signature rounds its target bar half up", "[timesig][move]")
{
    CHECK(moveTo(threeSections(), {1}, 1, 8640) == timeSigs({{1, 4, 4}, {6, 3, 4}, {9, 6, 8}}));
    CHECK(moveTo(threeSections(), {1}, 1, 8639) == threeSections());
    CHECK(moveTo(threeSections(), {1}, 1, 6720) == threeSections());
    CHECK(moveTo(threeSections(), {1}, 1, 6719) == timeSigs({{1, 4, 4}, {4, 3, 4}, {9, 6, 8}}));
}

TEST_CASE("a moved change stops one bar after the previous change", "[timesig][move]")
{
    CHECK(moveTo(threeSections(), {1}, 1, 0) == timeSigs({{1, 4, 4}, {2, 3, 4}, {9, 6, 8}}));
}

TEST_CASE("a moved change stops one bar before the next change", "[timesig][move]")
{
    CHECK(moveTo(threeSections(), {1}, 1, 100000) == timeSigs({{1, 4, 4}, {8, 3, 4}, {9, 6, 8}}));
}

TEST_CASE("the last change has no upper limit", "[timesig][move]")
{
    CHECK(moveTo(threeSections(), {2}, 2, 29280) == timeSigs({{1, 4, 4}, {5, 3, 4}, {20, 6, 8}}));
}

TEST_CASE("a group of time signatures keeps its spacing in bars", "[timesig][move]")
{
    CHECK(moveTo(threeSections(), {1, 2}, 1, 3840) == timeSigs({{1, 4, 4}, {3, 3, 4}, {7, 6, 8}}));
}

TEST_CASE("the anchor uses the slope of the whole group", "[timesig][move]")
{
    CHECK(moveTo(threeSections(), {1, 2}, 2, 9600) == timeSigs({{1, 4, 4}, {3, 3, 4}, {7, 6, 8}}));
}

TEST_CASE("the first change never moves even when selected", "[timesig][move]")
{
    CHECK(moveTo(threeSections(), {0, 1}, 1, 3840) == timeSigs({{1, 4, 4}, {3, 3, 4}, {9, 6, 8}}));
}

TEST_CASE("out-of-range moved indices are ignored", "[timesig][move]")
{
    CHECK(moveTo(threeSections(), {1, 7, -2}, 1, 3840) ==
          std::vector<TimeSignatureChange>{{0, 4, 4}, {3840, 3, 4}, {12480, 6, 8}});
}

TEST_CASE("invalid time signature anchors are a no-op", "[timesig][move]")
{
    CHECK(moveTo(threeSections(), {0}, 0, 3840) == threeSections());
    CHECK(moveTo(threeSections(), {1}, 3, 3840) == threeSections());
    CHECK(moveTo(threeSections(), {1}, -1, 3840) == threeSections());
    CHECK(moveTo(threeSections(), {2}, 1, 3840) == threeSections());
}

TEST_CASE("sparse selection between one-bar gaps does not move", "[timesig][move]")
{
    const auto before = timeSigs({{1, 4, 4}, {2, 3, 4}, {3, 4, 4}, {4, 3, 4}});
    CHECK(moveTo(before, {1, 3}, 1, 20000) == before);
    CHECK(moveTo(before, {1, 3}, 1, 0) == before);
}

TEST_CASE("a non-positive slope is a no-op", "[timesig][move]")
{
    const auto before = timeSigs({{1, 2, 4}, {2, 8, 4}, {4, 2, 4}, {5, 4, 4}});
    CHECK(moveTo(before, {1, 3}, 3, 20000) == before);
    CHECK(moveTo(before, {1, 3}, 3, 0) == before);
}
