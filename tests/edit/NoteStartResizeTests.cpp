#include "edit/NoteEdits.h"
#include "support/GridTestHelpers.h"
#include "support/MidiNoteTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace GridTest;

TEST_CASE("dragging the start left lengthens the note and keeps its end", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 960, 480}, -480, fourFourSixteenths) == MidiNote{60, 100, 480, 960});
}

TEST_CASE("dragging the start right shortens the note and keeps its end", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 960, 480}, 240, fourFourSixteenths) == MidiNote{60, 100, 1200, 240});
}

TEST_CASE("the start stops at the last line before the end", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 960, 480}, 960, fourFourSixteenths) == MidiNote{60, 100, 1320, 120});
}

TEST_CASE("the start does not go before tick 0", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 240, 480}, -960, fourFourSixteenths) == MidiNote{60, 100, 0, 720});
}

TEST_CASE("a note starting on the last line before its end keeps its start", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 960, 240}, 0, fourFourQuarters) == MidiNote{60, 100, 960, 240});
    CHECK(NoteEdits::afterStartResize({60, 100, 960, 240}, 120, fourFourQuarters) == MidiNote{60, 100, 960, 240});
}

TEST_CASE("a note ending before the first line after tick 0 can only start at tick 0", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 0, 240}, 0, fourFourQuarters) == MidiNote{60, 100, 0, 240});
    CHECK(NoteEdits::afterStartResize({60, 100, 0, 240}, 120, fourFourQuarters) == MidiNote{60, 100, 0, 240});
    CHECK(NoteEdits::afterStartResize({60, 100, 0, 240}, -120, fourFourQuarters) == MidiNote{60, 100, 0, 240});
    CHECK(NoteEdits::afterStartResize({60, 100, 100, 140}, 0, fourFourQuarters) == MidiNote{60, 100, 0, 240});
}

TEST_CASE("the start can stop at the line before a bar line after a short last step", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 960, 480}, 480, threeFourQuarterTriplets) ==
          MidiNote{60, 100, 1280, 160});
}
