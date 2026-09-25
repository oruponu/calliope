#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace timesigtest;

namespace
{
std::vector<TimeSignatureChange> threeSections()
{
    return timeSigs({{1, 4, 4}, {5, 3, 4}, {9, 6, 8}});
}

std::vector<TimeSignatureChange> deleteAt(const std::vector<TimeSignatureChange>& before, const std::set<int>& indices)
{
    return MidiSequence::buildTimeSignatureChangesAfterDelete(before, indices, ppq);
}
} // namespace

TEST_CASE("remaining changes keep their bar numbers", "[timesig][delete]")
{
    CHECK(deleteAt(threeSections(), {1}) == std::vector<TimeSignatureChange>{{0, 4, 4}, {15360, 6, 8}});
}

TEST_CASE("ticks after a deleted change are recomputed with the new bar lengths", "[timesig][delete]")
{
    const auto before = timeSigs({{1, 4, 4}, {3, 3, 4}, {5, 4, 4}, {7, 6, 8}});
    CHECK(deleteAt(before, {1}) == timeSigs({{1, 4, 4}, {5, 4, 4}, {7, 6, 8}}));
}

TEST_CASE("several changes can be deleted at once", "[timesig][delete]")
{
    CHECK(deleteAt(threeSections(), {1, 2}) == timeSigs({{1, 4, 4}}));
}

TEST_CASE("the first change is never deleted", "[timesig][delete]")
{
    CHECK(deleteAt(threeSections(), {0, 1}) == timeSigs({{1, 4, 4}, {9, 6, 8}}));
}

TEST_CASE("nothing to delete returns the input unchanged", "[timesig][delete]")
{
    CHECK(deleteAt(threeSections(), {}) == threeSections());
    CHECK(deleteAt(threeSections(), {-1, 0, 5}) == threeSections());
}

TEST_CASE("an empty list stays empty", "[timesig][delete]")
{
    CHECK(deleteAt({}, {0, 1}).empty());
}
