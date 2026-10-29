#include "session/LayeredNoteEdits.h"
#include "support/MidiNoteTestHelpers.h"
#include "undo/event/MultiNoteModifyAction.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
class FakeEditor : public RecordingLayerEditor
{
public:
    std::vector<std::vector<RecordedNoteEdit>> edits;
    std::vector<std::vector<int>> removals;

    void editRecordedNotes(const std::vector<RecordedNoteEdit>& recordedEdits) override
    {
        edits.push_back(recordedEdits);
    }
    void removeRecordedNotes(const std::vector<int>& ids) override { removals.push_back(ids); }
};

MidiTrack trackWith(std::vector<MidiNote> notes)
{
    MidiTrack track;
    for (const auto& note : notes)
        track.addNote(note);
    return track;
}

constexpr NoteRef recorded(int trackIndex, int id)
{
    return {trackIndex, id, NoteLayer::Recording};
}

void performUndoneMove(MidiSequence& seq, UndoHistory& history)
{
    history.beginNewTransaction("Move Note");
    history.perform(new MultiNoteModifyAction(&seq, {{0, 0, {60, 100, 0, 480}, {60, 100, 240, 480}}}));
    history.undo();
}
} // namespace

TEST_CASE("a mixed edit makes only the sequence side undoable", "[edit][layered-edits]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}}));
    UndoHistory history;
    FakeEditor editor;
    LayeredNoteEdits(seq, history, &editor)
        .apply({{{0, 0}, {.startTick = 240}}, {recorded(0, 7), {.startTick = 720}}}, "Move Notes");

    CHECK(seq.getTrack(0).getNotes() == std::vector<MidiNote>{{60, 100, 240, 480}});
    CHECK(editor.edits == std::vector<std::vector<RecordedNoteEdit>>{{{7, {.startTick = 720}}}});
    CHECK(history.getUndoDescription() == "Move Notes");

    history.undo();
    CHECK(seq.getTrack(0).getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
    CHECK(editor.edits.size() == 1);
}

TEST_CASE("an edit of recorded notes only leaves the undo history alone", "[edit][layered-edits]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}}));
    UndoHistory history;
    FakeEditor editor;
    performUndoneMove(seq, history);
    LayeredNoteEdits(seq, history, &editor).apply({{recorded(0, 3), {.velocity = 30}}}, "Edit Velocity");

    CHECK(editor.edits == std::vector<std::vector<RecordedNoteEdit>>{{{3, {.velocity = 30}}}});
    CHECK_FALSE(history.canUndo());
    CHECK(history.canRedo());
}

TEST_CASE("a sequence edit that changes nothing adds no undo step", "[edit][layered-edits]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}}));
    UndoHistory history;
    LayeredNoteEdits(seq, history, nullptr).apply({{{0, 0}, {.startTick = 0}}}, "Move Note");
    CHECK_FALSE(history.canUndo());
}

TEST_CASE("removing a mixed selection deletes the sequence side as one undo step", "[edit][layered-edits]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}, {62, 100, 480, 480}}));
    UndoHistory history;
    FakeEditor editor;
    LayeredNoteEdits(seq, history, &editor).remove({{0, 1}, recorded(0, 4)}, "Delete Notes");

    CHECK(seq.getTrack(0).getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
    CHECK(editor.removals == std::vector<std::vector<int>>{{4}});
    CHECK(history.getUndoDescription() == "Delete Notes");

    history.undo();
    CHECK(seq.getTrack(0).getNumNotes() == 2);
}

TEST_CASE("removing recorded notes only leaves the undo history alone", "[edit][layered-edits]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}}));
    UndoHistory history;
    FakeEditor editor;
    performUndoneMove(seq, history);
    LayeredNoteEdits(seq, history, &editor).remove({recorded(0, 4)}, "Delete Notes");

    CHECK(editor.removals == std::vector<std::vector<int>>{{4}});
    CHECK_FALSE(history.canUndo());
    CHECK(history.canRedo());
}

TEST_CASE("recorded edits are dropped without an editor", "[edit][layered-edits]")
{
    MidiSequence seq;
    seq.addTrack(trackWith({{60, 100, 0, 480}}));
    UndoHistory history;
    LayeredNoteEdits edits(seq, history, nullptr);
    edits.apply({{recorded(0, 3), {.velocity = 30}}}, "Edit Velocity");
    edits.remove({recorded(0, 3)}, "Delete Notes");

    CHECK(seq.getTrack(0).getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
    CHECK_FALSE(history.canUndo());
}
