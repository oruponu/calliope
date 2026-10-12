#include "engine/NoteFieldEdit.h"
#include "support/MidiNoteTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
const MidiNote note{60, 90, 100, 480};
} // namespace

TEST_CASE("a start edit keeps the length", "[engine][note-edit]")
{
    CHECK(NoteFieldEdit{.startTick = 240}.appliedTo(note) == MidiNote{60, 90, 240, 480});
}

TEST_CASE("a start edit that keeps the end changes the length", "[engine][note-edit]")
{
    CHECK(NoteFieldEdit{.startTick = 240, .keepEnd = true}.appliedTo(note) == MidiNote{60, 90, 240, 340});
}

TEST_CASE("an end edit changes the length", "[engine][note-edit]")
{
    CHECK(NoteFieldEdit{.endTick = 340}.appliedTo(note) == MidiNote{60, 90, 100, 240});
}

TEST_CASE("an end edit before the start leaves one tick", "[engine][note-edit]")
{
    CHECK(NoteFieldEdit{.endTick = 50}.appliedTo(note) == MidiNote{60, 90, 100, 1});
}

TEST_CASE("pitch and velocity edits set those fields", "[engine][note-edit]")
{
    CHECK(NoteFieldEdit{.noteNumber = 64, .velocity = 30}.appliedTo(note) == MidiNote{64, 30, 100, 480});
}

TEST_CASE("a held length leaves a finished note's length alone", "[engine][note-edit]")
{
    CHECK(NoteFieldEdit{.startTick = 240, .heldLength = 60}.appliedTo(note) == MidiNote{60, 90, 240, 480});
}
