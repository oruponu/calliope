#include "model/MidiNote.h"
#include "model/MidiSequence.h"
#include "model/TrackId.h"
#include "support/MidiNoteTestHelpers.h"
#include "undo/TrackActions.h"
#include <catch2/catch_test_macros.hpp>
#include <utility>
#include <vector>

TEST_CASE("redoing a track addition restores the same id", "[undo][track]")
{
    MidiSequence seq;
    seq.addTrack();
    TrackAddAction action(&seq);
    action.perform();
    REQUIRE(action.getAddedIndex() == 1);
    const TrackId added = seq.getTrack(1).getId();

    action.undo();
    CHECK(seq.getNumTracks() == 1);
    CHECK(seq.indexOf(added) == -1);

    action.perform();
    CHECK(seq.getNumTracks() == 2);
    CHECK(seq.getTrack(1).getId() == added);
}

TEST_CASE("redoing a track addition makes routes to it resolve again", "[undo][track]")
{
    MidiSequence seq;
    seq.addTrack();
    TrackAddAction action(&seq);
    action.perform();
    const TrackId added = seq.getTrack(1).getId();
    seq.setTrackRouteTarget(0, added);

    action.undo();
    CHECK(seq.resolveRouteTarget(0) == seq.getTrack(0).getId());

    action.perform();
    CHECK(seq.resolveRouteTarget(0) == added);
}

TEST_CASE("undoing a track removal restores the track at its index with its id", "[undo][track]")
{
    MidiSequence seq;
    const TrackId first = seq.addTrack().getId();
    const TrackId second = seq.addTrack().getId();
    const TrackId third = seq.addTrack().getId();
    TrackRemoveAction action(&seq, 1);
    action.perform();
    CHECK(seq.getNumTracks() == 2);
    CHECK(seq.indexOf(second) == -1);

    action.undo();
    CHECK(seq.indexOf(first) == 0);
    CHECK(seq.indexOf(second) == 1);
    CHECK(seq.indexOf(third) == 2);
}

TEST_CASE("undoing a track removal restores its contents", "[undo][track]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.setTrackName(1, "Bass");
    seq.setTrackChannel(1, 2);
    seq.addNote(1, {40, 90, 0, 480});
    seq.addNote(1, {43, 80, 480, 240});
    TrackRemoveAction action(&seq, 1);
    action.perform();
    action.undo();

    const MidiTrack& restored = seq.getTrack(1);
    CHECK(restored.getName() == "Bass");
    CHECK(restored.getChannel() == 2);
    CHECK(restored.getNotes() == std::vector<MidiNote>{{40, 90, 0, 480}, {43, 80, 480, 240}});
}

TEST_CASE("undoing a track removal makes routes to it resolve again", "[undo][track]")
{
    MidiSequence seq;
    seq.addTrack();
    const TrackId target = seq.addTrack().getId();
    seq.setTrackRouteTarget(0, target);
    TrackRemoveAction action(&seq, 1);
    action.perform();
    CHECK(seq.resolveRouteTarget(0) == seq.getTrack(0).getId());

    action.undo();
    CHECK(seq.resolveRouteTarget(0) == target);
}

TEST_CASE("a channel change reports the track before changing its channel", "[undo][track]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    std::vector<std::pair<int, int>> reports;
    ChannelChangeAction action(&seq, 1, 1, 5,
                               [&](int index) { reports.emplace_back(index, seq.getTrack(index).getChannel()); });

    action.perform();
    CHECK(seq.getTrack(1).getChannel() == 5);
    action.undo();
    CHECK(seq.getTrack(1).getChannel() == 1);
    CHECK(reports == std::vector<std::pair<int, int>>{{1, 1}, {1, 5}});
}
