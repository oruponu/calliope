#include "model/MidiSequence.h"
#include "model/TimedContents.h"
#include "support/MidiEventTestHelpers.h"
#include "support/MidiNoteTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

namespace
{
struct RecordingListener : MidiSequence::Listener
{
    std::vector<std::string> calls;

    void notesChanged(int trackIndex) override { calls.push_back("notes " + std::to_string(trackIndex)); }
    void tracksChanged() override { calls.push_back("tracks"); }
    void tempoChanged() override { calls.push_back("tempo"); }
    void timelineMetadataChanged() override { calls.push_back("metadata"); }
    void sequenceReset() override { calls.push_back("reset"); }
};

SequenceContents makeContents()
{
    SequenceContents contents;
    contents.timeline.setTempoChanges({{0, 120.0}, {960, 90.0}});
    contents.timeline.setTimeSignatureChanges({{0, 4, 4}, {1920, 3, 4}});
    contents.keySignatureChanges = {{0, 2, false}};
    contents.chordChanges = {ChordChange::noChord(480)};
    contents.metaEvents = {{0, 0x03, {'A'}}};

    MidiTrack first;
    first.addNote({60, 100, 0, 480});
    first.addNote({62, 100, 480, 480});
    first.addEvent({MidiEvent::Type::ControlChange, 240, 7, 90, false});
    first.addSysEx({100, {0x7E, 0x7F}});
    first.addMetaEvent({50, 0x06, {'m'}});
    MidiTrack second;
    second.addNote({72, 80, 960, 240});
    contents.tracks.push_back(first);
    contents.tracks.push_back(second);
    return contents;
}
} // namespace

TEST_CASE("timed contents round-trip through the sequence", "[timed-contents]")
{
    MidiSequence seq;
    seq.replaceContents(makeContents());

    TimedContents contents = seq.getTimedContents();
    REQUIRE(contents.tracks.size() == 2);
    contents.tempoChanges.push_back({1920, 100.0});
    contents.timeSignatureChanges = {{0, 4, 4}, {960, 6, 8}};
    contents.keySignatureChanges = {{0, -1, true}, {480, 3, false}};
    contents.chordChanges = {ChordChange::noChord(0), ChordChange::noChord(960)};
    contents.metaEvents = {{10, 0x01, {'x', 'y'}}};
    contents.tracks[0].notes = {{64, 90, 120, 60}};
    contents.tracks[0].events = {{MidiEvent::Type::ProgramChange, 5, 3, 0, false}};
    contents.tracks[0].sysExEvents = {{7, {0x01}}};
    contents.tracks[0].metaEvents = {{9, 0x02, {'z'}}};
    contents.tracks[1].notes.clear();

    seq.setTimedContents(contents);

    CHECK(seq.getTimedContents() == contents);
}

TEST_CASE("setting timed contents keeps track identity and settings", "[timed-contents]")
{
    MidiSequence seq;
    seq.replaceContents(makeContents());
    const TrackId other = seq.getTrack(1).getId();
    seq.setTrackName(0, "Lead");
    seq.setTrackChannel(0, 5);
    seq.setTrackMuted(0, true);
    seq.setTrackOutputDestination(0, MidiTrack::OutputDestination::None);
    seq.setTrackRouteTarget(0, other);
    const TrackId id = seq.getTrack(0).getId();

    TimedContents contents = seq.getTimedContents();
    contents.tracks[0].notes.clear();
    seq.setTimedContents(contents);

    CHECK(seq.getNumTracks() == 2);
    CHECK(seq.getTrack(0).getId() == id);
    CHECK(seq.getTrack(0).getName() == "Lead");
    CHECK(seq.getTrack(0).getChannel() == 5);
    CHECK(seq.getTrack(0).isMuted());
    CHECK(seq.getTrack(0).getOutputDestination() == MidiTrack::OutputDestination::None);
    CHECK(seq.getTrack(0).getRouteTarget() == other);
    CHECK(seq.getTrack(0).getNumNotes() == 0);
}

TEST_CASE("setting timed contents skips tracks that are gone", "[timed-contents]")
{
    MidiSequence seq;
    seq.replaceContents(makeContents());

    TimedContents contents = seq.getTimedContents();
    contents.tracks[1].notes = {{40, 70, 0, 120}};
    TrackTimedContents gone;
    gone.track = TrackId{9999};
    gone.notes = {{1, 1, 0, 1}};
    contents.tracks.insert(contents.tracks.begin(), gone);
    seq.setTimedContents(contents);

    CHECK(seq.getNumTracks() == 2);
    REQUIRE(seq.getTrack(1).getNumNotes() == 1);
    CHECK(seq.getTrack(1).getNote(0).noteNumber == 40);
    CHECK(seq.getTrack(0).getNumNotes() == 2);
}

TEST_CASE("setting timed contents sends one batch of notifications", "[timed-contents]")
{
    MidiSequence seq;
    seq.replaceContents(makeContents());
    RecordingListener listener;
    seq.addListener(&listener);

    seq.setTimedContents(seq.getTimedContents());

    CHECK(listener.calls == std::vector<std::string>{"notes -1", "tempo", "metadata"});
}
