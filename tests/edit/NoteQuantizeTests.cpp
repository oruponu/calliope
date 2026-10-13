#include "edit/NoteQuantize.h"
#include "support/MidiNoteTestHelpers.h"
#include "support/NoteEditStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <utility>
#include <vector>

TEST_CASE("a start in the first half of a grid step moves back to the earlier line", "[note][quantize]")
{
    const std::vector<std::pair<NoteRef, MidiNote>> notes{{{0, 0}, {60, 100, 130, 200}}};
    CHECK(NoteQuantize::startsToGrid(notes, 120) ==
          std::vector<NoteEdit>{{{0, 0}, {.startTick = 120, .heldLength = 200}}});
}

TEST_CASE("a start in the second half of a grid step moves on to the later line", "[note][quantize]")
{
    const std::vector<std::pair<NoteRef, MidiNote>> notes{{{0, 0}, {60, 100, 230, 200}}};
    CHECK(NoteQuantize::startsToGrid(notes, 120) ==
          std::vector<NoteEdit>{{{0, 0}, {.startTick = 240, .heldLength = 200}}});
}

TEST_CASE("a start exactly halfway between grid lines moves to the later line", "[note][quantize]")
{
    const std::vector<std::pair<NoteRef, MidiNote>> notes{{{0, 0}, {60, 100, 180, 200}}};
    CHECK(NoteQuantize::startsToGrid(notes, 120) ==
          std::vector<NoteEdit>{{{0, 0}, {.startTick = 240, .heldLength = 200}}});
}

TEST_CASE("a start less than half a grid step after tick 0 moves to tick 0", "[note][quantize]")
{
    const std::vector<std::pair<NoteRef, MidiNote>> notes{{{0, 0}, {60, 100, 59, 200}}};
    CHECK(NoteQuantize::startsToGrid(notes, 120) ==
          std::vector<NoteEdit>{{{0, 0}, {.startTick = 0, .heldLength = 200}}});
}

TEST_CASE("notes already on the grid get no edit", "[note][quantize]")
{
    const std::vector<std::pair<NoteRef, MidiNote>> notes{
        {{0, 0}, {60, 100, 240, 200}}, {{0, 1}, {64, 100, 250, 200}}, {{0, 2}, {67, 100, 0, 200}}};
    CHECK(NoteQuantize::startsToGrid(notes, 120) ==
          std::vector<NoteEdit>{{{0, 1}, {.startTick = 240, .heldLength = 200}}});
}

TEST_CASE("the edit carries the note length as the held length", "[note][quantize]")
{
    const MidiNote note{62, 90, 1000, 333};
    const auto edits = NoteQuantize::startsToGrid({{{0, 0}, note}}, 480);
    REQUIRE(edits.size() == 1);
    CHECK(edits[0].fields.heldLength == 333);
    CHECK_FALSE(edits[0].fields.keepEnd);
    CHECK(edits[0].fields.appliedTo(note) == MidiNote{62, 90, 960, 333});
}

TEST_CASE("recorded notes keep their recorded refs", "[note][quantize]")
{
    const NoteRef recorded{1, 7, NoteLayer::Recording};
    const std::vector<std::pair<NoteRef, MidiNote>> notes{{{1, 0}, {60, 100, 10, 50}}, {recorded, {64, 100, 470, 30}}};
    CHECK(NoteQuantize::startsToGrid(notes, 120) ==
          std::vector<NoteEdit>{{{1, 0}, {.startTick = 0, .heldLength = 50}},
                                {recorded, {.startTick = 480, .heldLength = 30}}});
}

TEST_CASE("a note that would end past the last tick moves to the earlier line", "[note][quantize]")
{
    constexpr int lastTick = std::numeric_limits<int>::max();
    const std::vector<std::pair<NoteRef, MidiNote>> notes{{{0, 0}, {60, 100, lastTick - 37, 30}}};
    CHECK(NoteQuantize::startsToGrid(notes, 120) ==
          std::vector<NoteEdit>{{{0, 0}, {.startTick = lastTick - 127, .heldLength = 30}}});
}

TEST_CASE("a note near the last tick moves later when its end still fits", "[note][quantize]")
{
    constexpr int lastTick = std::numeric_limits<int>::max();
    const std::vector<std::pair<NoteRef, MidiNote>> notes{{{0, 0}, {60, 100, lastTick - 37, 7}}};
    CHECK(NoteQuantize::startsToGrid(notes, 120) ==
          std::vector<NoteEdit>{{{0, 0}, {.startTick = lastTick - 7, .heldLength = 7}}});
}

TEST_CASE("a grid below one tick gives no edits", "[note][quantize]")
{
    const std::vector<std::pair<NoteRef, MidiNote>> notes{{{0, 0}, {60, 100, 130, 200}}};
    CHECK(NoteQuantize::startsToGrid(notes, 0).empty());
}

TEST_CASE("no notes give no edits", "[note][quantize]")
{
    CHECK(NoteQuantize::startsToGrid({}, 120).empty());
}
