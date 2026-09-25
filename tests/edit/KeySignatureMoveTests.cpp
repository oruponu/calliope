#include "edit/KeySignatureEdits.h"
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
    TimelineMap timeline;
    CHECK(KeySignatureEdits::afterMove(cgf(), {1}, 1, 3840, timeline) ==
          Keys{{0, 0, false}, {3840, 1, false}, {15360, -1, false}});
}

TEST_CASE("a moved key rounds its target bar half up", "[keysig][move]")
{
    TimelineMap timeline;
    CHECK(KeySignatureEdits::afterMove(cgf(), {1}, 1, 8640, timeline) ==
          Keys{{0, 0, false}, {9600, 1, false}, {15360, -1, false}});
    CHECK(KeySignatureEdits::afterMove(cgf(), {1}, 1, 8639, timeline) == cgf());
}

TEST_CASE("a moved key stops one bar after the previous key", "[keysig][move]")
{
    TimelineMap timeline;
    CHECK(KeySignatureEdits::afterMove(cgf(), {1}, 1, 0, timeline) ==
          Keys{{0, 0, false}, {1920, 1, false}, {15360, -1, false}});
}

TEST_CASE("a moved key stops one bar before the next key", "[keysig][move]")
{
    TimelineMap timeline;
    CHECK(KeySignatureEdits::afterMove(cgf(), {1}, 1, 100000, timeline) ==
          Keys{{0, 0, false}, {13440, 1, false}, {15360, -1, false}});
}

TEST_CASE("the first key can move", "[keysig][move]")
{
    TimelineMap timeline;
    CHECK(KeySignatureEdits::afterMove(cgf(), {0}, 0, 3840, timeline) ==
          Keys{{3840, 0, false}, {7680, 1, false}, {15360, -1, false}});
}

TEST_CASE("a negative target is treated as tick 0", "[keysig][move]")
{
    TimelineMap timeline;
    const Keys before{{3840, 0, false}, {7680, 1, false}};
    CHECK(KeySignatureEdits::afterMove(before, {0}, 0, -5000, timeline) == Keys{{0, 0, false}, {7680, 1, false}});
}

TEST_CASE("a key in the middle of a bar snaps to the bar start", "[keysig][move]")
{
    TimelineMap timeline;
    const Keys before{{0, 0, false}, {8160, 1, false}, {15360, -1, false}};
    CHECK(KeySignatureEdits::afterMove(before, {1}, 1, 8160, timeline) ==
          Keys{{0, 0, false}, {7680, 1, false}, {15360, -1, false}});
}

TEST_CASE("a key may move into the bar of a next key that is mid-bar", "[keysig][move]")
{
    TimelineMap timeline;
    const Keys before{{0, 0, false}, {3840, 1, false}, {8160, -1, false}};
    const Keys expected{{0, 0, false}, {7680, 1, false}, {8160, -1, false}};
    CHECK(KeySignatureEdits::afterMove(before, {1}, 1, 7680, timeline) == expected);
    CHECK(KeySignatureEdits::afterMove(before, {1}, 1, 100000, timeline) == expected);
}

TEST_CASE("no room between neighbors is a no-op", "[keysig][move]")
{
    TimelineMap timeline;
    const Keys before{{0, 0, false}, {8160, 1, false}, {8640, 2, false}, {9600, 3, false}};
    CHECK(KeySignatureEdits::afterMove(before, {2}, 2, 20000, timeline) == before);
    CHECK(KeySignatureEdits::afterMove(before, {2}, 2, 0, timeline) == before);
}

TEST_CASE("a group of keys keeps its spacing in bars", "[keysig][move]")
{
    TimelineMap timeline;
    CHECK(KeySignatureEdits::afterMove(cgf(), {1, 2}, 1, 3840, timeline) ==
          Keys{{0, 0, false}, {3840, 1, false}, {11520, -1, false}});
}

TEST_CASE("moved keys sharing a bar are a no-op", "[keysig][move]")
{
    TimelineMap timeline;
    const Keys before{{0, 0, false}, {7680, 1, false}, {8160, 2, false}, {15360, -1, false}};
    CHECK(KeySignatureEdits::afterMove(before, {1, 2}, 1, 3840, timeline) == before);
}

TEST_CASE("invalid key anchors are a no-op", "[keysig][move]")
{
    TimelineMap timeline;
    CHECK(KeySignatureEdits::afterMove(cgf(), {1}, -1, 3840, timeline) == cgf());
    CHECK(KeySignatureEdits::afterMove(cgf(), {1}, 3, 3840, timeline) == cgf());
    CHECK(KeySignatureEdits::afterMove(cgf(), {2}, 1, 3840, timeline) == cgf());
}

TEST_CASE("bars follow the time signature map", "[keysig][move]")
{
    TimelineMap timeline;
    setTimeSignatures(timeline, {{1, 4, 4}, {3, 3, 4}});
    const Keys before{{0, 0, false}, {6720, 1, false}};
    CHECK(KeySignatureEdits::afterMove(before, {1}, 1, 5999, timeline) == Keys{{0, 0, false}, {5280, 1, false}});
    CHECK(KeySignatureEdits::afterMove(before, {1}, 1, 6000, timeline) == before);
}
