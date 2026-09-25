#include "model/MidiSequence.h"
#include "support/TempoTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
using Tempos = std::vector<TempoChange>;
} // namespace

TEST_CASE("adding a tempo at an existing tick overwrites it and returns its index", "[tempo][add-change]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 120.0}, {1920, 90.0}});
    CHECK(seq.addTempoChange(1920, 80.0) == 1);
    CHECK(seq.getTempoChanges() == Tempos{{0, 120.0}, {1920, 80.0}});
}

TEST_CASE("adding a tempo between changes inserts it in tick order", "[tempo][add-change]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 120.0}, {1920, 90.0}});
    CHECK(seq.addTempoChange(960, 100.0) == 1);
    CHECK(seq.getTempoChanges() == Tempos{{0, 120.0}, {960, 100.0}, {1920, 90.0}});
}

TEST_CASE("adding a tempo after the last change appends it", "[tempo][add-change]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 120.0}, {1920, 90.0}});
    CHECK(seq.addTempoChange(3840, 150.0) == 2);
    CHECK(seq.getTempoChanges() == Tempos{{0, 120.0}, {1920, 90.0}, {3840, 150.0}});
}

TEST_CASE("adding a tempo before the first change prepends it", "[tempo][add-change]")
{
    MidiSequence seq;
    seq.setTempoChanges({{960, 100.0}});
    CHECK(seq.addTempoChange(0, 120.0) == 0);
    CHECK(seq.getTempoChanges() == Tempos{{0, 120.0}, {960, 100.0}});
}
