#include "edit/NoteEdits.h"
#include "support/MidiNoteTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("dragging the end right lengthens the note and keeps its start", "[note][end-resize]")
{
    CHECK(NoteEdits::afterEndResize({60, 100, 960, 480}, 480, 120) == MidiNote{60, 100, 960, 960});
}

TEST_CASE("dragging the end left shortens the note and keeps its start", "[note][end-resize]")
{
    CHECK(NoteEdits::afterEndResize({60, 100, 960, 480}, -240, 120) == MidiNote{60, 100, 960, 240});
}

TEST_CASE("the end stops the minimum length after the start", "[note][end-resize]")
{
    CHECK(NoteEdits::afterEndResize({60, 100, 960, 480}, -960, 120) == MidiNote{60, 100, 960, 120});
}

TEST_CASE("a note shorter than the minimum length grows to it toward the end", "[note][end-resize]")
{
    CHECK(NoteEdits::afterEndResize({60, 100, 960, 240}, 0, 480) == MidiNote{60, 100, 960, 480});
    CHECK(NoteEdits::afterEndResize({60, 100, 960, 240}, -120, 480) == MidiNote{60, 100, 960, 480});
}
