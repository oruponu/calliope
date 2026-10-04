#include "edit/KeySignatureEdits.h"
#include "support/KeySignatureStringMaker.h"
#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
using Keys = std::vector<KeySignatureChange>;

const TimelineMap timeline;
} // namespace

TEST_CASE("pasted key signatures land on the anchor bar plus their offsets", "[keysig][paste]")
{
    CHECK(KeySignatureEdits::afterPaste({{0, 0, false}}, {{0, 2, false}, {2, -1, true}}, 2, timeline) ==
          Keys{{0, 0, false}, {1920, 2, false}, {5760, -1, true}});
}

TEST_CASE("pasting onto an existing bar overwrites its key signature", "[keysig][paste]")
{
    CHECK(KeySignatureEdits::afterPaste({{0, 0, false}, {1920, 3, false}}, {{0, -2, true}}, 2, timeline) ==
          Keys{{0, 0, false}, {1920, -2, true}});
}

TEST_CASE("pasting at bar 1 overwrites the first key signature", "[keysig][paste]")
{
    CHECK(KeySignatureEdits::afterPaste({{0, 0, false}, {1920, 3, false}}, {{0, -2, true}}, 1, timeline) ==
          Keys{{0, -2, true}, {1920, 3, false}});
}

TEST_CASE("pasting between key signatures keeps them in tick order", "[keysig][paste]")
{
    CHECK(KeySignatureEdits::afterPaste({{0, 0, false}, {5760, 3, false}}, {{0, -2, true}}, 2, timeline) ==
          Keys{{0, 0, false}, {1920, -2, true}, {5760, 3, false}});
}

TEST_CASE("pasted key signatures follow the time signature map", "[keysig][paste]")
{
    TimelineMap threeFour;
    timesigtest::setTimeSignatures(threeFour, {{1, 3, 4}});
    CHECK(KeySignatureEdits::afterPaste({{0, 0, false}}, {{0, 2, false}}, 3, threeFour) ==
          Keys{{0, 0, false}, {2880, 2, false}});
}

TEST_CASE("pasting no key signatures is a no-op", "[keysig][paste]")
{
    CHECK(KeySignatureEdits::afterPaste({{0, 0, false}}, {}, 2, timeline) == Keys{{0, 0, false}});
}
