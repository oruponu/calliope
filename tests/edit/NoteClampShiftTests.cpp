#include "edit/NoteEdits.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
const std::vector<MidiNote> notes{{60, 100, 960, 480}, {120, 100, 480, 480}, {5, 100, 1440, 480}};
} // namespace

TEST_CASE("a shift that keeps every note in range is unchanged", "[note][clamp-shift]")
{
    const auto shift = NoteEdits::clampShift(notes, {-480, 7});
    CHECK(shift.deltaTick == -480);
    CHECK(shift.deltaNote == 7);
}

TEST_CASE("an earlier shift stops when the earliest note reaches tick 0", "[note][clamp-shift]")
{
    const auto shift = NoteEdits::clampShift(notes, {-1000, 0});
    CHECK(shift.deltaTick == -480);
    CHECK(shift.deltaNote == 0);
}

TEST_CASE("a later shift has no limit", "[note][clamp-shift]")
{
    const auto shift = NoteEdits::clampShift(notes, {1'000'000, 0});
    CHECK(shift.deltaTick == 1'000'000);
    CHECK(shift.deltaNote == 0);
}

TEST_CASE("an upward shift stops when the highest note reaches 127", "[note][clamp-shift]")
{
    const auto shift = NoteEdits::clampShift(notes, {0, 10});
    CHECK(shift.deltaTick == 0);
    CHECK(shift.deltaNote == 7);
}

TEST_CASE("a downward shift stops when the lowest note reaches 0", "[note][clamp-shift]")
{
    const auto shift = NoteEdits::clampShift(notes, {0, -10});
    CHECK(shift.deltaTick == 0);
    CHECK(shift.deltaNote == -5);
}

TEST_CASE("time and pitch are clamped independently", "[note][clamp-shift]")
{
    const auto shift = NoteEdits::clampShift(notes, {-1000, 10});
    CHECK(shift.deltaTick == -480);
    CHECK(shift.deltaNote == 7);
}
