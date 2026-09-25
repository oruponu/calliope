#include "support/ChordTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace chordtest;

TEST_CASE("adding a chord at an existing tick overwrites all of its fields", "[chord][add-change]")
{
    MidiSequence seq;
    seq.setChordChanges(chords({{0, "C"}, {1920, "N.C."}}));
    seq.addChordChange(0, 0x34, 10, 0x31, 0);
    CHECK(seq.getChordChanges() == std::vector<ChordChange>{{0, 0x34, 10, 0x31, 0}, chord(1920, "N.C.")});
}

TEST_CASE("adding a chord at a new tick inserts it in tick order", "[chord][add-change]")
{
    MidiSequence seq;
    seq.setChordChanges(chords({{0, "C"}, {3840, "N.C."}}));
    seq.addChordChange(1920, 0x35, 0, 0x7F, 0x7F);
    CHECK(seq.getChordChanges() == chords({{0, "C"}, {1920, "G"}, {3840, "N.C."}}));
}

TEST_CASE("adding a chord before the first one prepends it", "[chord][add-change]")
{
    MidiSequence seq;
    seq.setChordChanges(chords({{1920, "C"}}));
    seq.addChordChange(0, 0x32, 8, 0x7F, 0x7F);
    CHECK(seq.getChordChanges() == chords({{0, "Dm"}, {1920, "C"}}));
}
