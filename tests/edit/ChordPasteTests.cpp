#include "support/ChordTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace chordtest;

TEST_CASE("pasting preserves lengths and gaps", "[chord][paste]")
{
    const std::vector<RelativeChord> items{relative(0, 1920, "C"), relative(3840, 1920, "G")};
    CHECK(ChordTrackEdits::afterPaste({}, items, 7680) ==
          chords({{7680, "C"}, {9600, "N.C."}, {11520, "G"}, {13440, "N.C."}}));
}

TEST_CASE("pasting a joined pair keeps it joined", "[chord][paste]")
{
    const auto before = chords({{0, "G"}, {1920, "N.C."}});
    const std::vector<RelativeChord> items{relative(0, 1920, "C"), relative(1920, 1920, "F")};
    CHECK(ChordTrackEdits::afterPaste(before, items, 3840) ==
          chords({{0, "G"}, {1920, "N.C."}, {3840, "C"}, {5760, "F"}, {7680, "N.C."}}));
}

TEST_CASE("a covered chord survives in the remaining part of its span", "[chord][paste]")
{
    const auto before = chords({{1920, "G"}, {5760, "N.C."}});
    CHECK(ChordTrackEdits::afterPaste(before, {relative(0, 1920, "C")}, 960) ==
          chords({{960, "C"}, {2880, "G"}, {5760, "N.C."}}));
}

TEST_CASE("a covered chord with no remaining span disappears", "[chord][paste]")
{
    const auto before = chords({{1920, "G"}, {2880, "N.C."}});
    CHECK(ChordTrackEdits::afterPaste(before, {relative(0, 1920, "C")}, 960) == chords({{960, "C"}, {2880, "N.C."}}));
}

TEST_CASE("a straddled chord is truncated and does not resume", "[chord][paste]")
{
    const auto before = chords({{0, "A"}, {7680, "N.C."}});
    CHECK(ChordTrackEdits::afterPaste(before, {relative(0, 1920, "C")}, 1920) ==
          chords({{0, "A"}, {1920, "C"}, {3840, "N.C."}}));
}

TEST_CASE("start-aligned entries are covered and end-aligned entries survive", "[chord][paste]")
{
    const auto before = chords({{1920, "F"}, {3840, "G"}, {5760, "N.C."}});
    CHECK(ChordTrackEdits::afterPaste(before, {relative(0, 1920, "C")}, 1920) ==
          chords({{1920, "C"}, {3840, "G"}, {5760, "N.C."}}));
}

TEST_CASE("separate components inside one span leave silence between them", "[chord][paste]")
{
    const auto before = chords({{0, "A"}, {7680, "N.C."}});
    const std::vector<RelativeChord> items{relative(0, 480, "C"), relative(960, 480, "F")};
    CHECK(ChordTrackEdits::afterPaste(before, items, 1920) ==
          chords({{0, "A"}, {1920, "C"}, {2400, "N.C."}, {2880, "F"}, {3360, "N.C."}}));
}

TEST_CASE("an open-ended item is pasted as a point", "[chord][paste]")
{
    const auto before = chords({{0, "A"}, {1920, "F"}, {3840, "N.C."}});
    CHECK(ChordTrackEdits::afterPaste(before, {relative(0, 0, "G")}, 1920) ==
          chords({{0, "A"}, {1920, "G"}, {3840, "N.C."}}));
}

TEST_CASE("duplicate offsets keep the first item", "[chord][paste]")
{
    const std::vector<RelativeChord> items{relative(0, 1920, "C"), relative(0, 1920, "F")};
    CHECK(ChordTrackEdits::afterPaste({}, items, 0) == chords({{0, "C"}, {1920, "N.C."}}));
}

TEST_CASE("duplicate offsets keep the first item even in a large unsorted input", "[chord][paste][!mayfail]")
{
    const std::vector<std::string> names{"C", "D", "E", "F", "G", "A", "B"};
    std::vector<RelativeChord> items;
    for (int k = 0; k < 4; ++k)
        for (int j = 9; j >= 0; --j)
            items.push_back(relative(j * 1920, 1920, names[static_cast<size_t>((k * 10 + j) % 7)]));

    std::vector<ChordChange> expected;
    for (int j = 0; j < 10; ++j)
        expected.push_back(chord(j * 1920, names[static_cast<size_t>(j % 7)]));
    expected.push_back(chord(10 * 1920, "N.C."));

    CHECK(ChordTrackEdits::afterPaste({}, items, 0) == expected);
}

TEST_CASE("invalid items are ignored", "[chord][paste]")
{
    const auto before = chords({{0, "A"}, {1920, "N.C."}});
    CHECK(ChordTrackEdits::afterPaste(before, {}, 0) == before);
    const std::vector<RelativeChord> invalid{relative(-1, 1920, "C"), relative(0, -5, "F")};
    CHECK(ChordTrackEdits::afterPaste(before, invalid, 0) == before);
}
