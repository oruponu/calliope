#include "edit/TempoEdits.h"
#include "support/TempoStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
using Tempos = std::vector<TempoChange>;
} // namespace

TEST_CASE("pasted tempo changes land at the anchor plus their offsets", "[tempo][paste]")
{
    CHECK(TempoEdits::afterPaste({{0, 120.0}}, {{0, 90.0}, {480, 100.0}}, 960) ==
          Tempos{{0, 120.0}, {960, 90.0}, {1440, 100.0}});
}

TEST_CASE("pasting onto an existing tick overwrites its tempo", "[tempo][paste]")
{
    CHECK(TempoEdits::afterPaste({{0, 120.0}, {960, 80.0}}, {{0, 90.0}}, 960) == Tempos{{0, 120.0}, {960, 90.0}});
}

TEST_CASE("pasting at tick 0 overwrites the first tempo", "[tempo][paste]")
{
    CHECK(TempoEdits::afterPaste({{0, 120.0}, {960, 80.0}}, {{0, 90.0}}, 0) == Tempos{{0, 90.0}, {960, 80.0}});
}

TEST_CASE("pasting between tempo changes keeps them in tick order", "[tempo][paste]")
{
    CHECK(TempoEdits::afterPaste({{0, 120.0}, {1920, 80.0}}, {{0, 90.0}}, 960) ==
          Tempos{{0, 120.0}, {960, 90.0}, {1920, 80.0}});
}

TEST_CASE("pasting no tempo changes is a no-op", "[tempo][paste]")
{
    CHECK(TempoEdits::afterPaste({{0, 120.0}}, {}, 960) == Tempos{{0, 120.0}});
}
