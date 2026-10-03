#include "edit/NoteEdits.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("notes can be shifted up until the highest reaches 127", "[note][shift]")
{
    const std::vector<MidiNote> notes{{60, 100, 0, 480}, {120, 100, 480, 480}};
    CHECK(NoteEdits::canShiftPitch(notes, 7));
    CHECK_FALSE(NoteEdits::canShiftPitch(notes, 8));
}

TEST_CASE("notes can be shifted down until the lowest reaches 0", "[note][shift]")
{
    const std::vector<MidiNote> notes{{60, 100, 0, 480}, {5, 100, 480, 480}};
    CHECK(NoteEdits::canShiftPitch(notes, -5));
    CHECK_FALSE(NoteEdits::canShiftPitch(notes, -6));
}

TEST_CASE("notes can be shifted earlier until the earliest starts at tick 0", "[note][shift]")
{
    const std::vector<MidiNote> notes{{60, 100, 960, 480}, {64, 100, 480, 480}};
    CHECK(NoteEdits::canShiftTime(notes, -480));
    CHECK_FALSE(NoteEdits::canShiftTime(notes, -481));
}

TEST_CASE("notes can always be shifted later", "[note][shift]")
{
    const std::vector<MidiNote> notes{{60, 100, 0, 480}};
    CHECK(NoteEdits::canShiftTime(notes, 1'000'000));
}
