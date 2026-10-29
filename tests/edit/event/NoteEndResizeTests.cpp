#include "edit/event/NoteEdits.h"
#include "support/GridTestHelpers.h"
#include "support/MidiNoteTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace GridTest;

TEST_CASE("dragging the end right lengthens the note and keeps its start", "[note][end-resize]")
{
    CHECK(NoteEdits::afterEndResize({60, 100, 960, 480}, 480, fourFourSixteenths) == MidiNote{60, 100, 960, 960});
}

TEST_CASE("dragging the end left shortens the note and keeps its start", "[note][end-resize]")
{
    CHECK(NoteEdits::afterEndResize({60, 100, 960, 480}, -240, fourFourSixteenths) == MidiNote{60, 100, 960, 240});
}

TEST_CASE("the end stops at the first line after the start", "[note][end-resize]")
{
    CHECK(NoteEdits::afterEndResize({60, 100, 960, 480}, -960, fourFourSixteenths) == MidiNote{60, 100, 960, 120});
}

TEST_CASE("a note ending before the first line after its start grows to that line", "[note][end-resize]")
{
    CHECK(NoteEdits::afterEndResize({60, 100, 960, 240}, 0, fourFourQuarters) == MidiNote{60, 100, 960, 480});
    CHECK(NoteEdits::afterEndResize({60, 100, 960, 240}, -120, fourFourQuarters) == MidiNote{60, 100, 960, 480});
}

TEST_CASE("an off-grid start lets the end stop at the next line", "[note][end-resize]")
{
    CHECK(NoteEdits::afterEndResize({60, 100, 1300, 480}, -480, fourFourQuarters) == MidiNote{60, 100, 1300, 140});
}

TEST_CASE("the end can stop at the bar line after a short last step", "[note][end-resize]")
{
    CHECK(NoteEdits::afterEndResize({60, 100, 1280, 320}, -320, threeFourQuarterTriplets) ==
          MidiNote{60, 100, 1280, 160});
}

TEST_CASE("away from the bar end the end still stops a full step after the start", "[note][end-resize]")
{
    CHECK(NoteEdits::afterEndResize({60, 100, 640, 320}, -320, threeFourQuarterTriplets) ==
          MidiNote{60, 100, 640, 320});
}
