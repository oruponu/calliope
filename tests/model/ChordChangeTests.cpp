#include "support/ChordTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
constexpr int none = ChordChange::none;
} // namespace

TEST_CASE("a chord with a note root is not no chord whatever its type", "[model][chord]")
{
    CHECK_FALSE(ChordChange{0, 0x31, 0, none, none}.isNoChord());
    CHECK_FALSE(ChordChange{0, 0x23, 10, none, none}.isNoChord());
    CHECK_FALSE(ChordChange{0, 0x31, 0, 0x35, 0}.isNoChord());
    CHECK_FALSE(ChordChange{0, 0x31, 35, none, none}.isNoChord());
}

TEST_CASE("a missing root, an empty note or the no chord type is no chord", "[model][chord]")
{
    CHECK(ChordChange{0, none, 0, none, none}.isNoChord());
    CHECK(ChordChange{0, 0x30, 0, none, none}.isNoChord());
    CHECK(ChordChange{0, 0x00, 0, none, none}.isNoChord());
    CHECK(ChordChange{0, 0x31, ChordChange::typeCount, none, none}.isNoChord());
}

TEST_CASE("a root outside the note range is not no chord", "[model][chord]")
{
    CHECK_FALSE(ChordChange{0, 0x71, 0, none, none}.isNoChord());
    CHECK_FALSE(ChordChange{0, 0x38, 0, none, none}.isNoChord());
}

TEST_CASE("the canonical no chord and the XF terminator are no chord", "[model][chord]")
{
    CHECK(ChordChange::noChord(1920) == ChordChange{1920, none, ChordChange::typeCount, none, none});
    CHECK(ChordChange::noChord(1920).isNoChord());
    CHECK(ChordChange{0, none, 0x7F, none, none}.isNoChord());
}
