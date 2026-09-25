#include "support/ChordTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace chordtest;

namespace
{
std::vector<ChordChange> add(const std::vector<ChordChange>& before, int startTick, int endTick,
                             const std::string& name)
{
    const auto c = chord(0, name);
    return ChordTrackEdits::afterAdd(before, startTick, endTick, c.chordRoot, c.chordType, c.bassRoot, c.bassType);
}
} // namespace

TEST_CASE("adding to an empty track fills the span and adds a terminator", "[chord][add]")
{
    CHECK(add({}, 1920, 3840, "F") == chords({{1920, "F"}, {3840, "N.C."}}));
}

TEST_CASE("adding to the next bar replaces the previous terminator", "[chord][add]")
{
    const auto before = chords({{0, "C"}, {1920, "N.C."}});
    CHECK(add(before, 1920, 3840, "F") == chords({{0, "C"}, {1920, "F"}, {3840, "N.C."}}));
}

TEST_CASE("adding before the first chord in the same bar needs no terminator", "[chord][add]")
{
    const auto before = chords({{960, "C"}, {1920, "N.C."}});
    CHECK(add(before, 0, 960, "F") == chords({{0, "F"}, {960, "C"}, {1920, "N.C."}}));
}

TEST_CASE("adding into a gap wider than the span adds a terminator", "[chord][add]")
{
    const auto before = chords({{0, "C"}, {1920, "N.C."}, {5760, "G"}, {7680, "N.C."}});
    CHECK(add(before, 1920, 3840, "F") == chords({{0, "C"}, {1920, "F"}, {3840, "N.C."}, {5760, "G"}, {7680, "N.C."}}));
}

TEST_CASE("adding into a gap that ends at the span end joins the next chord", "[chord][add]")
{
    const auto before = chords({{0, "C"}, {1920, "N.C."}, {3840, "G"}, {5760, "N.C."}});
    CHECK(add(before, 1920, 3840, "F") == chords({{0, "C"}, {1920, "F"}, {3840, "G"}, {5760, "N.C."}}));
}

TEST_CASE("adding after consecutive no-chords does not backfill the earlier one", "[chord][add]")
{
    const auto before = chords({{0, "C"}, {1920, "N.C."}, {2400, "N.C."}});
    CHECK(add(before, 2400, 3840, "F") == chords({{0, "C"}, {1920, "N.C."}, {2400, "F"}, {3840, "N.C."}}));
}

TEST_CASE("the same chord can be added next to itself", "[chord][add]")
{
    const auto before = chords({{0, "C"}, {1920, "N.C."}});
    CHECK(add(before, 1920, 3840, "C") == chords({{0, "C"}, {1920, "C"}, {3840, "N.C."}}));
}

TEST_CASE("bass fields are written as given", "[chord][add]")
{
    const auto after = ChordTrackEdits::afterAdd({}, 0, 1920, 0x31, 0, 0x33, 0);
    const std::vector<ChordChange> expected{{0, 0x31, 0, 0x33, 0}, chord(1920, "N.C.")};
    CHECK(after == expected);
}

TEST_CASE("empty or inverted span is a no-op", "[chord][add]")
{
    const auto before = chords({{0, "C"}, {1920, "N.C."}});
    CHECK(add(before, 1920, 1920, "F") == before);
    CHECK(add(before, 3840, 1920, "F") == before);
}
