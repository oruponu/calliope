#include "edit/TempoEdits.h"
#include "support/TempoStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
using Tempos = std::vector<TempoChange>;

const Tempos before{{0, 120.0}, {960, 90.0}, {1920, 140.0}};
} // namespace

TEST_CASE("deleting removes the selected tempo changes", "[tempo][delete]")
{
    CHECK(TempoEdits::afterDelete(before, {1}) == Tempos{{0, 120.0}, {1920, 140.0}});
}

TEST_CASE("several tempo changes can be deleted at once", "[tempo][delete]")
{
    CHECK(TempoEdits::afterDelete(before, {1, 2}) == Tempos{{0, 120.0}});
}

TEST_CASE("the tempo change at tick 0 is never deleted", "[tempo][delete]")
{
    CHECK(TempoEdits::afterDelete(before, {0, 1}) == Tempos{{0, 120.0}, {1920, 140.0}});
}

TEST_CASE("out-of-range tempo indices are ignored", "[tempo][delete]")
{
    CHECK(TempoEdits::afterDelete(before, {-1, 3}) == before);
}

TEST_CASE("deleting nothing returns the tempo changes unchanged", "[tempo][delete]")
{
    CHECK(TempoEdits::afterDelete(before, {}) == before);
}
