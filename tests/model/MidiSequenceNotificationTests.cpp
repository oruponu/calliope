#include "model/MidiSequence.h"
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>
#include <utility>
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

const MidiNote note{60, 100, 0, 480};
} // namespace

TEST_CASE("adding, inserting and removing tracks each notify tracksChanged once", "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addListener(&listener);

    seq.addTrack();
    const MidiTrack saved = seq.getTrack(0);
    seq.removeTrack(0);
    seq.insertTrack(0, saved);

    CHECK(listener.calls == std::vector<std::string>{"tracks", "tracks", "tracks"});
}

TEST_CASE("each track property setter notifies tracksChanged once", "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addTrack();
    seq.addListener(&listener);

    seq.setTrackMuted(0, true);
    seq.setTrackSolo(0, true);
    seq.setTrackName(0, "Lead");
    seq.setTrackChannel(0, 2);
    seq.setTrackOutputDestination(0, MidiTrack::OutputDestination::None);
    seq.setTrackRouteTarget(0, std::nullopt);
    seq.setTrackPluginAssignment(0, nullptr);

    CHECK(listener.calls == std::vector<std::string>(7, "tracks"));
    CHECK(seq.getTrack(0).isMuted());
    CHECK(seq.getTrack(0).isSolo());
    CHECK(seq.getTrack(0).getName() == "Lead");
    CHECK(seq.getTrack(0).getChannel() == 2);
    CHECK(seq.getTrack(0).getOutputDestination() == MidiTrack::OutputDestination::None);
}

TEST_CASE("note edits notify notesChanged with the track index", "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.addListener(&listener);

    seq.addNote(1, note);
    seq.insertNote(1, 0, {62, 100, 0, 480});
    seq.setNote(1, 1, {64, 90, 480, 240});
    seq.removeNote(1, 0);

    CHECK(listener.calls == std::vector<std::string>(4, "notes 1"));
    REQUIRE(seq.getTrack(1).getNumNotes() == 1);
    CHECK(seq.getTrack(1).getNote(0).noteNumber == 64);
    CHECK(seq.getTrack(1).getNote(0).velocity == 90);
    CHECK(seq.getTrack(1).getNote(0).startTick == 480);
    CHECK(seq.getTrack(1).getNote(0).duration == 240);
}

TEST_CASE("setting tempo changes notifies tempoChanged", "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addListener(&listener);

    seq.setTempoChanges({{0, 90.0}});

    CHECK(listener.calls == std::vector<std::string>{"tempo"});
}

TEST_CASE("setting time signatures, key signatures and chords notifies timelineMetadataChanged",
          "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addListener(&listener);

    seq.setTimeSignatureChanges({{0, 3, 4}});
    seq.setKeySignatureChanges({{0, 2, false}});
    seq.setChordChanges({ChordChange::noChord(0)});

    CHECK(listener.calls == std::vector<std::string>(3, "metadata"));
}

TEST_CASE("a batch holds notifications until it ends", "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addTrack();
    seq.addListener(&listener);

    {
        MidiSequence::ChangeBatch batch(seq);
        seq.setTrackMuted(0, true);
        seq.addNote(0, note);
        CHECK(listener.calls.empty());
    }

    CHECK(listener.calls == std::vector<std::string>{"tracks", "notes 0"});
}

TEST_CASE("a batch notifies each kind once in a fixed order", "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addTrack();
    seq.addListener(&listener);

    {
        MidiSequence::ChangeBatch batch(seq);
        seq.setChordChanges({ChordChange::noChord(0)});
        seq.setTempoChanges({{0, 90.0}});
        seq.addNote(0, note);
        seq.setTrackMuted(0, true);
        seq.setTempoChanges({{0, 100.0}});
        seq.setTrackSolo(0, true);
    }

    CHECK(listener.calls == std::vector<std::string>{"tracks", "notes 0", "tempo", "metadata"});
}

TEST_CASE("a batch that changes notes in one track passes that track's index", "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.addListener(&listener);

    {
        MidiSequence::ChangeBatch batch(seq);
        seq.addNote(1, note);
        seq.addNote(1, note);
    }

    CHECK(listener.calls == std::vector<std::string>{"notes 1"});
}

TEST_CASE("a batch that changes notes in two tracks passes -1", "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.addListener(&listener);

    {
        MidiSequence::ChangeBatch batch(seq);
        seq.addNote(0, note);
        seq.addNote(1, note);
    }

    CHECK(listener.calls == std::vector<std::string>{"notes -1"});
}

TEST_CASE("a batch that changes notes and removes a track passes -1", "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.addListener(&listener);

    {
        MidiSequence::ChangeBatch batch(seq);
        seq.addNote(1, note);
        seq.removeTrack(0);
    }

    CHECK(listener.calls == std::vector<std::string>{"tracks", "notes -1"});
}

TEST_CASE("nested batches notify only when the outermost batch ends", "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addTrack();
    seq.addListener(&listener);

    {
        MidiSequence::ChangeBatch outer(seq);
        {
            MidiSequence::ChangeBatch inner(seq);
            seq.setTrackMuted(0, true);
        }
        CHECK(listener.calls.empty());
        seq.setTrackSolo(0, true);
    }

    CHECK(listener.calls == std::vector<std::string>{"tracks"});
}

TEST_CASE("replacing contents notifies sequenceReset once", "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addListener(&listener);

    SequenceContents contents;
    contents.tracks.emplace_back();
    contents.tracks.emplace_back();
    seq.replaceContents(std::move(contents));

    CHECK(listener.calls == std::vector<std::string>{"reset"});
    CHECK(seq.getNumTracks() == 2);
}

TEST_CASE("replacing contents inside a batch notifies only sequenceReset", "[model][notification]")
{
    RecordingListener listener;
    MidiSequence seq;
    seq.addTrack();
    seq.addListener(&listener);

    {
        MidiSequence::ChangeBatch batch(seq);
        seq.setTrackMuted(0, true);
        seq.replaceContents({});
        seq.setTempoChanges({{0, 90.0}});
    }

    CHECK(listener.calls == std::vector<std::string>{"reset"});
}

TEST_CASE("replacing contents renumbers route targets and drops unknown ones", "[model][sequence]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();

    MidiSequence source;
    source.addTrack();
    source.addTrack();
    source.addTrack();
    source.setTrackRouteTarget(0, source.getTrack(1).getId());
    source.setTrackRouteTarget(1, source.getTrack(2).getId());

    SequenceContents contents;
    contents.tracks = {source.getTrack(0), source.getTrack(1)};
    seq.replaceContents(std::move(contents));

    REQUIRE(seq.getNumTracks() == 2);
    CHECK(seq.getTrack(0).getRouteTarget() == std::optional<TrackId>{seq.getTrack(1).getId()});
    CHECK_FALSE(seq.getTrack(1).getRouteTarget().has_value());
}
