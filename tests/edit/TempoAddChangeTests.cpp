#include "edit/TempoEdits.h"
#include "support/TempoTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
using Tempos = std::vector<TempoChange>;
} // namespace

TEST_CASE("adding a tempo at an existing tick overwrites it and returns its index", "[tempo][add-change]")
{
    Tempos changes{{0, 120.0}, {1920, 90.0}};
    CHECK(TempoEdits::add(changes, 1920, 80.0) == 1);
    CHECK(changes == Tempos{{0, 120.0}, {1920, 80.0}});
}

TEST_CASE("adding a tempo between changes inserts it in tick order", "[tempo][add-change]")
{
    Tempos changes{{0, 120.0}, {1920, 90.0}};
    CHECK(TempoEdits::add(changes, 960, 100.0) == 1);
    CHECK(changes == Tempos{{0, 120.0}, {960, 100.0}, {1920, 90.0}});
}

TEST_CASE("adding a tempo after the last change appends it", "[tempo][add-change]")
{
    Tempos changes{{0, 120.0}, {1920, 90.0}};
    CHECK(TempoEdits::add(changes, 3840, 150.0) == 2);
    CHECK(changes == Tempos{{0, 120.0}, {1920, 90.0}, {3840, 150.0}});
}

TEST_CASE("adding a tempo before the first change prepends it", "[tempo][add-change]")
{
    Tempos changes{{960, 100.0}};
    CHECK(TempoEdits::add(changes, 0, 120.0) == 0);
    CHECK(changes == Tempos{{0, 120.0}, {960, 100.0}});
}
