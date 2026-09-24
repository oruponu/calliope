#include "support/ChordTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace chordtest;

namespace
{
std::vector<ChordChange> joinedRun()
{
    return chords({{0, "C"}, {1920, "F"}, {3840, "G"}, {5760, "N.C."}});
}
} // namespace

TEST_CASE("deleting from a joined run leaves a gap and keeps the previous length", "[chord][delete]")
{
    CHECK(MidiSequence::buildChordChangesAfterDelete(joinedRun(), {1}) ==
          chords({{0, "C"}, {1920, "N.C."}, {3840, "G"}, {5760, "N.C."}}));
}

TEST_CASE("deleting a chord with a terminator merges the gaps", "[chord][delete]")
{
    const auto before = chords({{0, "C"}, {1920, "F"}, {3840, "N.C."}, {5760, "G"}, {7680, "N.C."}});
    CHECK(MidiSequence::buildChordChangesAfterDelete(before, {1}) ==
          chords({{0, "C"}, {1920, "N.C."}, {5760, "G"}, {7680, "N.C."}}));
}

TEST_CASE("deleting a joined tail patches only once", "[chord][delete]")
{
    const auto expected = chords({{0, "C"}, {1920, "N.C."}});
    CHECK(MidiSequence::buildChordChangesAfterDelete(joinedRun(), {1, 2}) == expected);
    CHECK(MidiSequence::buildChordChangesAfterDelete(joinedRun(), {2, 1, 1}) == expected);
}

TEST_CASE("deleting the first chord needs no patch", "[chord][delete]")
{
    const auto before = chords({{0, "C"}, {1920, "F"}, {3840, "N.C."}});
    CHECK(MidiSequence::buildChordChangesAfterDelete(before, {0}) == chords({{1920, "F"}, {3840, "N.C."}}));
}

TEST_CASE("deleting an open-ended last chord patches its start", "[chord][delete]")
{
    const auto before = chords({{0, "C"}, {1920, "F"}});
    CHECK(MidiSequence::buildChordChangesAfterDelete(before, {1}) == chords({{0, "C"}, {1920, "N.C."}}));
}

TEST_CASE("deleting after a no-chord needs no patch", "[chord][delete]")
{
    const auto before = chords({{0, "C"}, {1920, "N.C."}, {3840, "F"}, {5760, "N.C."}});
    CHECK(MidiSequence::buildChordChangesAfterDelete(before, {2}) == chords({{0, "C"}, {1920, "N.C."}}));
}

TEST_CASE("deleting every chord leaves an empty track", "[chord][delete]")
{
    const auto before = chords({{0, "C"}, {1920, "F"}, {3840, "N.C."}});
    CHECK(MidiSequence::buildChordChangesAfterDelete(before, {0, 1}).empty());
}

TEST_CASE("deletion normalizes redundant no-chords elsewhere", "[chord][delete]")
{
    const auto before = chords({{0, "C"}, {1920, "N.C."}, {2400, "N.C."}, {3840, "F"}, {5760, "N.C."}});
    CHECK(MidiSequence::buildChordChangesAfterDelete(before, {3}) == chords({{0, "C"}, {1920, "N.C."}}));
}

TEST_CASE("invalid or no-chord indices are a no-op", "[chord][delete]")
{
    const auto before = chords({{0, "C"}, {1920, "N.C."}, {3840, "F"}, {5760, "N.C."}});
    CHECK(MidiSequence::buildChordChangesAfterDelete(before, {}) == before);
    CHECK(MidiSequence::buildChordChangesAfterDelete(before, {-1, 99}) == before);
    CHECK(MidiSequence::buildChordChangesAfterDelete(before, {1, 3}) == before);
}
