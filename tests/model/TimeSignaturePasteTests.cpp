#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace timesigtest;

namespace
{
std::vector<TimeSignatureChange> threeSections()
{
    return timeSigs({{1, 4, 4}, {5, 3, 4}, {9, 6, 8}});
}

std::vector<TimeSignatureChange> paste(const std::vector<TimeSignatureChange>& before,
                                       const std::vector<RelativeTimeSignature>& items, int anchorBar)
{
    return MidiSequence::buildTimeSignatureChangesAfterPaste(before, items, anchorBar, ppq);
}
} // namespace

TEST_CASE("pasted changes land at the anchor bar plus their offsets", "[timesig][paste]")
{
    CHECK(paste(threeSections(), {{0, 3, 4}, {4, 6, 8}}, 13) ==
          std::vector<TimeSignatureChange>{{0, 4, 4}, {7680, 3, 4}, {13440, 6, 8}, {19200, 3, 4}, {24960, 6, 8}});
}

TEST_CASE("pasting onto an existing bar overwrites it", "[timesig][paste]")
{
    CHECK(paste(threeSections(), {{0, 2, 4}}, 5) == timeSigs({{1, 4, 4}, {5, 2, 4}, {9, 6, 8}}));
}

TEST_CASE("pasting at bar 1 overwrites the first change", "[timesig][paste]")
{
    CHECK(paste(threeSections(), {{0, 3, 4}}, 1) == timeSigs({{1, 3, 4}, {5, 3, 4}, {9, 6, 8}}));
}

TEST_CASE("pasting between changes inserts and shifts the following ticks", "[timesig][paste]")
{
    CHECK(paste(threeSections(), {{0, 2, 4}}, 7) == timeSigs({{1, 4, 4}, {5, 3, 4}, {7, 2, 4}, {9, 6, 8}}));
}

TEST_CASE("items before bar 1 are skipped", "[timesig][paste]")
{
    CHECK(paste(threeSections(), {{0, 2, 4}, {1, 3, 8}}, 0) == timeSigs({{1, 3, 8}, {5, 3, 4}, {9, 6, 8}}));
}

TEST_CASE("empty input is a no-op", "[timesig][paste]")
{
    CHECK(paste(threeSections(), {}, 5) == threeSections());
    CHECK(paste({}, {{0, 2, 4}}, 5).empty());
}
