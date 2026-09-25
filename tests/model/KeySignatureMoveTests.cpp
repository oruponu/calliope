#include "support/KeySignatureTestHelpers.h"
#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace timesigtest;

namespace
{
using Keys = std::vector<KeySignatureChange>;

Keys cgf()
{
    return {{0, 0, false}, {7680, 1, false}, {15360, -1, false}};
}
} // namespace

TEST_CASE("a moved key lands on the target bar", "[keysig][move]")
{
    MidiSequence seq;
    CHECK(seq.buildKeySignatureChangesAfterMove(cgf(), {1}, 1, 3840) ==
          Keys{{0, 0, false}, {3840, 1, false}, {15360, -1, false}});
}

TEST_CASE("a moved key rounds its target bar half up", "[keysig][move]")
{
    MidiSequence seq;
    CHECK(seq.buildKeySignatureChangesAfterMove(cgf(), {1}, 1, 8640) ==
          Keys{{0, 0, false}, {9600, 1, false}, {15360, -1, false}});
    CHECK(seq.buildKeySignatureChangesAfterMove(cgf(), {1}, 1, 8639) == cgf());
}

TEST_CASE("a moved key stops one bar after the previous key", "[keysig][move]")
{
    MidiSequence seq;
    CHECK(seq.buildKeySignatureChangesAfterMove(cgf(), {1}, 1, 0) ==
          Keys{{0, 0, false}, {1920, 1, false}, {15360, -1, false}});
}

TEST_CASE("a moved key stops one bar before the next key", "[keysig][move]")
{
    MidiSequence seq;
    CHECK(seq.buildKeySignatureChangesAfterMove(cgf(), {1}, 1, 100000) ==
          Keys{{0, 0, false}, {13440, 1, false}, {15360, -1, false}});
}

TEST_CASE("the first key can move", "[keysig][move]")
{
    MidiSequence seq;
    CHECK(seq.buildKeySignatureChangesAfterMove(cgf(), {0}, 0, 3840) ==
          Keys{{3840, 0, false}, {7680, 1, false}, {15360, -1, false}});
}

TEST_CASE("a negative target is treated as tick 0", "[keysig][move]")
{
    MidiSequence seq;
    const Keys before{{3840, 0, false}, {7680, 1, false}};
    CHECK(seq.buildKeySignatureChangesAfterMove(before, {0}, 0, -5000) == Keys{{0, 0, false}, {7680, 1, false}});
}

TEST_CASE("a key in the middle of a bar snaps to the bar start", "[keysig][move]")
{
    MidiSequence seq;
    const Keys before{{0, 0, false}, {8160, 1, false}, {15360, -1, false}};
    CHECK(seq.buildKeySignatureChangesAfterMove(before, {1}, 1, 8160) ==
          Keys{{0, 0, false}, {7680, 1, false}, {15360, -1, false}});
}

TEST_CASE("a key may move into the bar of a next key that is mid-bar", "[keysig][move]")
{
    MidiSequence seq;
    const Keys before{{0, 0, false}, {3840, 1, false}, {8160, -1, false}};
    const Keys expected{{0, 0, false}, {7680, 1, false}, {8160, -1, false}};
    CHECK(seq.buildKeySignatureChangesAfterMove(before, {1}, 1, 7680) == expected);
    CHECK(seq.buildKeySignatureChangesAfterMove(before, {1}, 1, 100000) == expected);
}

TEST_CASE("no room between neighbors is a no-op", "[keysig][move]")
{
    MidiSequence seq;
    const Keys before{{0, 0, false}, {8160, 1, false}, {8640, 2, false}, {9600, 3, false}};
    CHECK(seq.buildKeySignatureChangesAfterMove(before, {2}, 2, 20000) == before);
    CHECK(seq.buildKeySignatureChangesAfterMove(before, {2}, 2, 0) == before);
}

TEST_CASE("a group of keys keeps its spacing in bars", "[keysig][move]")
{
    MidiSequence seq;
    CHECK(seq.buildKeySignatureChangesAfterMove(cgf(), {1, 2}, 1, 3840) ==
          Keys{{0, 0, false}, {3840, 1, false}, {11520, -1, false}});
}

TEST_CASE("moved keys sharing a bar are a no-op", "[keysig][move]")
{
    MidiSequence seq;
    const Keys before{{0, 0, false}, {7680, 1, false}, {8160, 2, false}, {15360, -1, false}};
    CHECK(seq.buildKeySignatureChangesAfterMove(before, {1, 2}, 1, 3840) == before);
}

TEST_CASE("invalid key anchors are a no-op", "[keysig][move]")
{
    MidiSequence seq;
    CHECK(seq.buildKeySignatureChangesAfterMove(cgf(), {1}, -1, 3840) == cgf());
    CHECK(seq.buildKeySignatureChangesAfterMove(cgf(), {1}, 3, 3840) == cgf());
    CHECK(seq.buildKeySignatureChangesAfterMove(cgf(), {2}, 1, 3840) == cgf());
}

TEST_CASE("bars follow the time signature map", "[keysig][move]")
{
    MidiSequence seq;
    setTimeSignatures(seq, {{1, 4, 4}, {3, 3, 4}});
    const Keys before{{0, 0, false}, {6720, 1, false}};
    CHECK(seq.buildKeySignatureChangesAfterMove(before, {1}, 1, 5999) == Keys{{0, 0, false}, {5280, 1, false}});
    CHECK(seq.buildKeySignatureChangesAfterMove(before, {1}, 1, 6000) == before);
}
