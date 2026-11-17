#include "edit/time/InsertSilence.h"
#include "model/MidiSequence.h"
#include "undo/ReplaceTimedContentsAction.h"
#include "undo/UndoHistory.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
TimedContents song(TrackId track)
{
    TimedContents contents;
    contents.ticksPerQuarterNote = 480;
    contents.tempoChanges = {{0, 120.0}};
    contents.timeSignatureChanges = {{0, 4, 4}};
    contents.metaEvents = {{3840, 0x01, {'a'}}};
    contents.tracks.push_back({});
    contents.tracks[0].track = track;
    contents.tracks[0].notes = {{60, 100, 3840, 100}};
    return contents;
}
} // namespace

TEST_CASE("replacing timed contents is undone and redone", "[undo][time]")
{
    MidiSequence seq;
    const TimedContents before = song(seq.addTrack().getId());
    seq.setTimedContents(before);
    TimedContents after = before;
    after.tracks[0].notes[0].startTick = 7680;
    ReplaceTimedContentsAction action(&seq, before, after);

    CHECK(action.perform());
    CHECK(seq.getTimedContents() == after);

    CHECK(action.undo());
    CHECK(seq.getTimedContents() == before);

    CHECK(action.perform());
    CHECK(seq.getTimedContents() == after);
}

TEST_CASE("undoing a silence insertion through the history restores the song in one step", "[undo][time]")
{
    MidiSequence seq;
    const TimedContents before = song(seq.addTrack().getId());
    seq.setTimedContents(before);
    const auto after = insertSilence(before, 1920, 3840);
    REQUIRE(after);
    UndoHistory history;

    performReplaceTimedContents(history, &seq, "Insert Silence", before, *after);

    CHECK(seq.getTimedContents() == *after);
    CHECK(history.getUndoDescription() == "Insert Silence");

    history.undo();

    CHECK(seq.getTimedContents() == before);
    CHECK(history.getRedoDescription() == "Insert Silence");
}
