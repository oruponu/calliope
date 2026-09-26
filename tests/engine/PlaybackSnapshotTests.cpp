#include "engine/PlaybackSnapshot.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("notes are sorted by startTick and carry resolved ctx", "[engine][snapshot]")
{
    MidiSequence seq;
    auto& t0 = seq.addTrack();
    t0.setChannel(3);
    t0.addNote({60, 100, 480, 480});
    t0.addNote({62, 100, 0, 240});

    const auto snap = PlaybackSnapshot::build(seq);
    REQUIRE(snap.notes.size() == 2);
    CHECK(snap.notes[0].note.startTick == 0);
    CHECK(snap.notes[1].note.startTick == 480);
    CHECK(snap.notes[0].ctx.channel == 3);
    CHECK(snap.notes[0].ctx.trackId == seq.getTrack(0).getId());
}

TEST_CASE("muted track is excluded", "[engine][snapshot]")
{
    MidiSequence seq;
    auto& t0 = seq.addTrack();
    t0.addNote({60, 100, 0, 480});
    auto& t1 = seq.addTrack();
    t1.setMuted(true);
    t1.addNote({64, 100, 0, 480});

    const auto snap = PlaybackSnapshot::build(seq);
    REQUIRE(snap.notes.size() == 1);
    CHECK(snap.notes[0].ctx.trackId == seq.getTrack(0).getId());
}

TEST_CASE("solo excludes non-solo tracks", "[engine][snapshot]")
{
    MidiSequence seq;
    auto& t0 = seq.addTrack();
    t0.addNote({60, 100, 0, 480});
    auto& t1 = seq.addTrack();
    t1.setSolo(true);
    t1.addNote({64, 100, 0, 480});

    const auto snap = PlaybackSnapshot::build(seq);
    REQUIRE(snap.notes.size() == 1);
    CHECK(snap.notes[0].ctx.trackId == seq.getTrack(1).getId());
}

TEST_CASE("mute wins over solo on the same track", "[engine][snapshot]")
{
    MidiSequence seq;
    auto& t0 = seq.addTrack();
    t0.addNote({60, 100, 0, 480});
    auto& t1 = seq.addTrack();
    t1.setMuted(true);
    t1.setSolo(true);
    t1.addNote({64, 100, 0, 480});
    auto& t2 = seq.addTrack();
    t2.setSolo(true);
    t2.addNote({67, 100, 0, 480});

    const auto snap = PlaybackSnapshot::build(seq);
    REQUIRE(snap.notes.size() == 1);
    CHECK(snap.notes[0].ctx.trackId == seq.getTrack(2).getId());
}

TEST_CASE("routeTarget carries the id of an existing target track", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.getTrack(0).setRouteTarget(seq.getTrack(1).getId());
    seq.getTrack(0).addNote({60, 100, 0, 480});

    const auto snap = PlaybackSnapshot::build(seq);
    REQUIRE(snap.notes.size() == 1);
    CHECK(snap.notes[0].ctx.routeTarget == seq.getTrack(1).getId());
}

TEST_CASE("routeTarget falls back to own track when the target was removed", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.getTrack(0).setRouteTarget(seq.getTrack(1).getId());
    seq.removeTrack(1);
    seq.getTrack(0).addNote({60, 100, 0, 480});

    const auto snap = PlaybackSnapshot::build(seq);
    REQUIRE(snap.notes.size() == 1);
    CHECK(snap.notes[0].ctx.routeTarget == seq.getTrack(0).getId());
}

TEST_CASE("getTempoAt returns last change at or before tick", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTempoChanges({{0, 120.0}, {960, 140.0}});

    const auto snap = PlaybackSnapshot::build(seq);
    CHECK_THAT(snap.getTempoAt(0), WithinAbs(120.0, 1e-9));
    CHECK_THAT(snap.getTempoAt(959), WithinAbs(120.0, 1e-9));
    CHECK_THAT(snap.getTempoAt(960), WithinAbs(140.0, 1e-9));
    CHECK_THAT(snap.getTempoAt(2000), WithinAbs(140.0, 1e-9));
}

TEST_CASE("getTempoAt defaults to 120 when there are no tempo changes", "[engine][snapshot]")
{
    const PlaybackSnapshot snap;
    REQUIRE(snap.tempoChanges.empty());
    CHECK_THAT(snap.getTempoAt(0), WithinAbs(120.0, 1e-9));
    CHECK_THAT(snap.getTempoAt(10000), WithinAbs(120.0, 1e-9));
}
