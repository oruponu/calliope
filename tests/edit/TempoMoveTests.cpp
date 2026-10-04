#include "edit/TempoEdits.h"
#include "support/TempoStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
using Tempos = std::vector<TempoChange>;

constexpr int grid = 480;
const Tempos before{{0, 120.0}, {1920, 100.0}, {3840, 140.0}};
} // namespace

TEST_CASE("a dragged tempo change moves to the target tick and tempo", "[tempo][move]")
{
    CHECK(TempoEdits::afterMove(before, {1}, 1, 2400, 110.0, grid) == Tempos{{0, 120.0}, {2400, 110.0}, {3840, 140.0}});
}

TEST_CASE("the tempo change at tick 0 changes only its tempo", "[tempo][move]")
{
    CHECK(TempoEdits::afterMove(before, {0}, 0, 960, 90.0, grid) == Tempos{{0, 90.0}, {1920, 100.0}, {3840, 140.0}});
}

TEST_CASE("a moved tempo change stops one grid after the previous change", "[tempo][move]")
{
    CHECK(TempoEdits::afterMove(before, {1}, 1, 0, 100.0, grid) == Tempos{{0, 120.0}, {480, 100.0}, {3840, 140.0}});
}

TEST_CASE("a moved tempo change stops one grid before the next change", "[tempo][move]")
{
    CHECK(TempoEdits::afterMove(before, {1}, 1, 5000, 100.0, grid) == Tempos{{0, 120.0}, {3360, 100.0}, {3840, 140.0}});
}

TEST_CASE("the last tempo change has no upper limit", "[tempo][move]")
{
    CHECK(TempoEdits::afterMove(before, {2}, 2, 10000, 140.0, grid) ==
          Tempos{{0, 120.0}, {1920, 100.0}, {10000, 140.0}});
}

TEST_CASE("no room between neighbors keeps the tick of a tempo change", "[tempo][move]")
{
    const Tempos tight{{0, 120.0}, {480, 100.0}, {960, 140.0}};
    CHECK(TempoEdits::afterMove(tight, {1}, 1, 2000, 100.0, grid) == tight);
}

TEST_CASE("a group of tempo changes keeps its spacing", "[tempo][move]")
{
    CHECK(TempoEdits::afterMove(before, {1, 2}, 1, 2880, 100.0, grid) ==
          Tempos{{0, 120.0}, {2880, 100.0}, {4800, 140.0}});
}

TEST_CASE("a group stops one grid after the unselected previous change", "[tempo][move]")
{
    CHECK(TempoEdits::afterMove(before, {1, 2}, 1, 0, 100.0, grid) == Tempos{{0, 120.0}, {480, 100.0}, {2400, 140.0}});
}

TEST_CASE("every tempo in a group shifts by the dragged change's tempo difference", "[tempo][move]")
{
    CHECK(TempoEdits::afterMove(before, {0, 1}, 1, 2400, 110.0, grid) ==
          Tempos{{0, 130.0}, {2400, 110.0}, {3840, 140.0}});
}

TEST_CASE("the tempo of a group stays within the allowed range", "[tempo][move]")
{
    CHECK(TempoEdits::afterMove(before, {1, 2}, 1, 1920, 1000.0, grid) ==
          Tempos{{0, 120.0}, {1920, 360.0}, {3840, 400.0}});
    CHECK(TempoEdits::afterMove(before, {1, 2}, 1, 1920, 0.0, grid) == Tempos{{0, 120.0}, {1920, 10.0}, {3840, 50.0}});
}

TEST_CASE("an invalid tempo anchor is a no-op", "[tempo][move]")
{
    CHECK(TempoEdits::afterMove(before, {1}, 3, 2400, 110.0, grid) == before);
    CHECK(TempoEdits::afterMove(before, {1}, -1, 2400, 110.0, grid) == before);
}
