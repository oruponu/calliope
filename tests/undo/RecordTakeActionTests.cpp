#include "model/MidiSequence.h"
#include "support/MidiEventTestHelpers.h"
#include "support/MidiNoteTestHelpers.h"
#include "support/NoteRefStringMaker.h"
#include "undo/RecordTakeAction.h"
#include <catch2/catch_test_macros.hpp>
#include <map>
#include <vector>

namespace
{
MidiEvent ccAt(int tick, int value)
{
    return {.type = MidiEvent::Type::ControlChange, .tick = tick, .data1 = 7, .data2 = value};
}

MidiTrack trackWith(std::vector<MidiNote> notes, std::vector<MidiEvent> events)
{
    MidiTrack track;
    for (const auto& note : notes)
        track.addNote(note);
    for (const auto& event : events)
        track.addEvent(event);
    return track;
}
} // namespace

TEST_CASE("a recorded take is added and removed again by undo", "[undo][record]")
{
    MidiSequence seq;
    const TrackId id = seq.addTrack(trackWith({{60, 100, 0, 480}}, {ccAt(0, 10), ccAt(960, 30)})).getId();
    RecordTakeAction action(&seq, Take{{id, {{{64, 90, 240, 240}}, {ccAt(480, 20), ccAt(1440, 40)}}}});

    action.perform();
    CHECK(seq.getTrack(0).getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}, {64, 90, 240, 240}});
    CHECK(seq.getTrack(0).getEvents() ==
          std::vector<MidiEvent>{ccAt(0, 10), ccAt(480, 20), ccAt(960, 30), ccAt(1440, 40)});

    action.undo();
    CHECK(seq.getTrack(0).getNotes() == std::vector<MidiNote>{{60, 100, 0, 480}});
    CHECK(seq.getTrack(0).getEvents() == std::vector<MidiEvent>{ccAt(0, 10), ccAt(960, 30)});

    action.perform();
    CHECK(seq.getTrack(0).getNumNotes() == 2);
    CHECK(seq.getTrack(0).getEvents() ==
          std::vector<MidiEvent>{ccAt(0, 10), ccAt(480, 20), ccAt(960, 30), ccAt(1440, 40)});
}

TEST_CASE("recorded events from several passes go in tick order after existing ones", "[undo][record]")
{
    MidiSequence seq;
    const TrackId id = seq.addTrack(trackWith({}, {ccAt(100, 0)})).getId();
    RecordTakeAction action(&seq, Take{{id, {{}, {ccAt(900, 1), ccAt(100, 2), ccAt(100, 3)}}}});

    action.perform();
    CHECK(seq.getTrack(0).getEvents() ==
          std::vector<MidiEvent>{ccAt(100, 0), ccAt(100, 2), ccAt(100, 3), ccAt(900, 1)});

    action.undo();
    CHECK(seq.getTrack(0).getEvents() == std::vector<MidiEvent>{ccAt(100, 0)});
}

TEST_CASE("a recorded take is written by track id after indices shift", "[undo][record]")
{
    MidiSequence seq;
    seq.addTrack();
    const TrackId second = seq.addTrack().getId();
    RecordTakeAction action(&seq, Take{{second, {{{64, 90, 0, 120}}, {}}}});

    seq.removeTrack(0);
    action.perform();
    CHECK(seq.getTrack(0).getNotes() == std::vector<MidiNote>{{64, 90, 0, 120}});

    action.undo();
    CHECK(seq.getTrack(0).getNumNotes() == 0);
}

TEST_CASE("a track that no longer exists is skipped", "[undo][record]")
{
    MidiSequence seq;
    const TrackId kept = seq.addTrack().getId();
    const TrackId removed = seq.addTrack().getId();
    seq.removeTrack(1);
    RecordTakeAction action(&seq, Take{{kept, {{{60, 90, 0, 120}}, {}}}, {removed, {{{62, 90, 0, 120}}, {}}}});

    action.perform();
    CHECK(seq.getNumTracks() == 1);
    CHECK(seq.getTrack(0).getNotes() == std::vector<MidiNote>{{60, 90, 0, 120}});

    action.undo();
    CHECK(seq.getTrack(0).getNumNotes() == 0);
}

TEST_CASE("a take across two tracks is added and undone together", "[undo][record]")
{
    MidiSequence seq;
    const TrackId piano = seq.addTrack().getId();
    const TrackId bass = seq.addTrack().getId();
    RecordTakeAction action(&seq, Take{{piano, {{{60, 90, 0, 240}}, {}}}, {bass, {{{40, 90, 240, 240}}, {}}}});

    action.perform();
    CHECK(seq.getTrack(0).getNumNotes() == 1);
    CHECK(seq.getTrack(1).getNumNotes() == 1);

    action.undo();
    CHECK(seq.getTrack(0).getNumNotes() == 0);
    CHECK(seq.getTrack(1).getNumNotes() == 0);
}

TEST_CASE("each recorded note id is mapped to where it was added", "[undo][record]")
{
    MidiSequence seq;
    const TrackId first = seq.addTrack(trackWith({{60, 100, 0, 480}, {62, 100, 480, 480}}, {})).getId();
    const TrackId second = seq.addTrack().getId();
    RecordTakeAction action(&seq, Take{{first, {{{64, 90, 0, 240}, {65, 90, 240, 240}}, {}, {5, 3}}},
                                       {second, {{{67, 90, 0, 240}}, {}, {8}}}});

    action.perform();
    CHECK(action.getCommittedNotes() == std::map<int, NoteRef>{{3, {0, 3}}, {5, {0, 2}}, {8, {1, 0}}});
}

TEST_CASE("a recorded track that no longer exists maps no ids", "[undo][record]")
{
    MidiSequence seq;
    seq.addTrack();
    RecordTakeAction action(&seq, Take{{TrackId{99}, {{{64, 90, 0, 240}}, {}, {5}}}});

    action.perform();
    CHECK(action.getCommittedNotes().empty());
}
