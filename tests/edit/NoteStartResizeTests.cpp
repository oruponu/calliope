#include "edit/NoteEdits.h"
#include "support/MidiNoteTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("dragging the start left lengthens the note and keeps its end", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 960, 480}, -480, 120) == MidiNote{60, 100, 480, 960});
}

TEST_CASE("dragging the start right shortens the note and keeps its end", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 960, 480}, 240, 120) == MidiNote{60, 100, 1200, 240});
}

TEST_CASE("the start stops the minimum length before the end", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 960, 480}, 960, 120) == MidiNote{60, 100, 1320, 120});
}

TEST_CASE("the start does not go before tick 0", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 240, 480}, -960, 120) == MidiNote{60, 100, 0, 720});
}

TEST_CASE("a note shorter than the minimum length grows to it toward the start", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 960, 240}, 0, 480) == MidiNote{60, 100, 720, 480});
    CHECK(NoteEdits::afterStartResize({60, 100, 960, 240}, 120, 480) == MidiNote{60, 100, 720, 480});
}

TEST_CASE("a note that ends before the minimum length grows only to tick 0", "[note][start-resize]")
{
    CHECK(NoteEdits::afterStartResize({60, 100, 0, 240}, 0, 480) == MidiNote{60, 100, 0, 240});
    CHECK(NoteEdits::afterStartResize({60, 100, 0, 240}, 120, 480) == MidiNote{60, 100, 0, 240});
    CHECK(NoteEdits::afterStartResize({60, 100, 0, 240}, -120, 480) == MidiNote{60, 100, 0, 240});
    CHECK(NoteEdits::afterStartResize({60, 100, 100, 140}, 0, 480) == MidiNote{60, 100, 0, 240});
}
