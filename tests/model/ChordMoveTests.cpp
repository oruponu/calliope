#include "support/ChordTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace chordtest;

namespace
{
std::vector<ChordChange> single()
{
    return chords({{0, "C"}, {1920, "F"}, {3840, "G"}});
}

std::vector<ChordChange> group()
{
    return chords({{0, "C"}, {1920, "F"}, {3840, "N.C."}, {5760, "G"}, {7680, "N.C."}});
}

std::vector<ChordChange> moveChords(const std::vector<ChordChange>& before, const std::vector<int>& moved, int anchor,
                                    int target)
{
    return MidiSequence::buildChordChangesAfterMove(before, moved, anchor, target, grid);
}
} // namespace

TEST_CASE("moving onto the next chord pushes it back by the moved length", "[chord][move]")
{
    CHECK(moveChords(single(), {1}, 1, 3840) == chords({{0, "C"}, {1920, "N.C."}, {3840, "F"}, {5760, "G"}}));
}

TEST_CASE("partially covering the next chord trims it from the left", "[chord][move]")
{
    CHECK(moveChords(single(), {1}, 1, 2880) == chords({{0, "C"}, {1920, "N.C."}, {2880, "F"}, {4800, "G"}}));
}

TEST_CASE("moving past the next chord leaves a gap at the origin", "[chord][move]")
{
    CHECK(moveChords(single(), {1}, 1, 7680) ==
          chords({{0, "C"}, {1920, "N.C."}, {3840, "G"}, {7680, "F"}, {9600, "N.C."}}));
}

TEST_CASE("moving left into the previous chord truncates it", "[chord][move]")
{
    CHECK(moveChords(single(), {1}, 1, 960) == chords({{0, "C"}, {960, "F"}, {2880, "N.C."}, {3840, "G"}}));
}

TEST_CASE("moving over the whole previous chord overwrites it", "[chord][move]")
{
    CHECK(moveChords(single(), {1}, 1, 0) == chords({{0, "F"}, {1920, "N.C."}, {3840, "G"}}));
}

TEST_CASE("move target snaps to the nearest grid", "[chord][move]")
{
    CHECK(moveChords(single(), {1}, 1, 3900) == moveChords(single(), {1}, 1, 3840));
    CHECK(moveChords(single(), {1}, 1, 3960) ==
          chords({{0, "C"}, {1920, "N.C."}, {3840, "G"}, {4080, "F"}, {6000, "N.C."}}));
}

TEST_CASE("open-ended chord moves as a point", "[chord][move]")
{
    CHECK(moveChords(single(), {2}, 2, 960) == chords({{0, "C"}, {960, "G"}, {1920, "F"}, {3840, "N.C."}}));
}

TEST_CASE("terminator fields are preserved when moving", "[chord][move]")
{
    const std::vector<ChordChange> before{chord(0, "C"), chord(1920, "N.C."), chord(3840, "F"), xfNoChord(5760)};
    const std::vector<ChordChange> expected{chord(0, "C"), chord(1920, "N.C."), chord(7680, "F"), xfNoChord(9600)};
    CHECK(moveChords(before, {2}, 2, 7680) == expected);
}

TEST_CASE("group move of a joined run closes the gap and joins the next chord", "[chord][move][group]")
{
    CHECK(moveChords(group(), {0, 1}, 0, 1920) == chords({{1920, "C"}, {3840, "F"}, {5760, "G"}, {7680, "N.C."}}));
}

TEST_CASE("group move that fully covers a chord removes it", "[chord][move][group]")
{
    CHECK(moveChords(group(), {0, 1}, 0, 3840) == chords({{3840, "C"}, {5760, "F"}, {7680, "N.C."}}));
}

TEST_CASE("sparse group move keeps the unselected chord in place", "[chord][move][group]")
{
    CHECK(moveChords(group(), {0, 3}, 0, 3840) ==
          chords({{1920, "F"}, {3840, "C"}, {5760, "N.C."}, {9600, "G"}, {11520, "N.C."}}));
}

TEST_CASE("group is clamped so the leftmost chord lands at 0", "[chord][move][group]")
{
    CHECK(moveChords(group(), {1, 3}, 3, 0) == chords({{0, "F"}, {1920, "N.C."}, {3840, "G"}, {5760, "N.C."}}));
    CHECK(moveChords(group(), {0, 1}, 1, 0) == group());
}

TEST_CASE("move is a no-op in degenerate cases", "[chord][move]")
{
    const auto before = group();
    CHECK(moveChords(before, {1}, 1, 1920) == before);
    CHECK(moveChords(before, {1}, 1, 2000) == before);
    CHECK(moveChords(before, {1}, 0, 3840) == before);
    CHECK(moveChords(before, {2}, 2, 5760) == before);
    CHECK(moveChords(before, {-1, 99}, 1, 3840) == before);
    CHECK(MidiSequence::buildChordChangesAfterMove(before, {1}, 1, 3840, 0) == before);
}
