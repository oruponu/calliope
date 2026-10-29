#include "engine/chase/Chase.h"
#include "engine/playback/PlaybackSnapshot.h"
#include "support/MidiEventTestHelpers.h"
#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("notes are sorted by startTick and carry resolved ctx", "[engine][snapshot]")
{
    MidiSequence seq;
    MidiTrack t0;
    t0.setChannel(3);
    t0.addNote({60, 100, 480, 480});
    t0.addNote({62, 100, 0, 240});
    seq.addTrack(t0);

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
    MidiTrack t0;
    t0.addNote({60, 100, 0, 480});
    seq.addTrack(t0);
    MidiTrack t1;
    t1.setMuted(true);
    t1.addNote({64, 100, 0, 480});
    seq.addTrack(t1);

    const auto snap = PlaybackSnapshot::build(seq);
    REQUIRE(snap.notes.size() == 1);
    CHECK(snap.notes[0].ctx.trackId == seq.getTrack(0).getId());
}

TEST_CASE("sysex of audible tracks are scheduled by tick in track order", "[engine][snapshot][sysex]")
{
    MidiSequence seq;
    MidiTrack t0;
    t0.addSysEx({480, {0x01}});
    seq.addTrack(t0);
    MidiTrack t1;
    t1.addSysEx({0, {0x02}});
    t1.addSysEx({480, {0x03}});
    seq.addTrack(t1);

    const auto snap = PlaybackSnapshot::build(seq);

    REQUIRE(snap.sysExEvents.size() == 3);
    CHECK(snap.sysExEvents[0].sysEx.data == std::vector<std::uint8_t>{0x02});
    CHECK(snap.sysExEvents[0].ctx.trackId == seq.getTrack(1).getId());
    CHECK(snap.sysExEvents[1].sysEx.data == std::vector<std::uint8_t>{0x01});
    CHECK(snap.sysExEvents[1].ctx.trackId == seq.getTrack(0).getId());
    CHECK(snap.sysExEvents[2].sysEx.data == std::vector<std::uint8_t>{0x03});
    CHECK(snap.sysExEvents[2].ctx.trackId == seq.getTrack(1).getId());
}

TEST_CASE("a muted track's sysex is not scheduled", "[engine][snapshot][sysex]")
{
    MidiSequence seq;
    MidiTrack track;
    track.setMuted(true);
    track.addSysEx({0, {0x01}});
    seq.addTrack(track);

    CHECK(PlaybackSnapshot::build(seq).sysExEvents.empty());
}

TEST_CASE("solo excludes non-solo tracks", "[engine][snapshot]")
{
    MidiSequence seq;
    MidiTrack t0;
    t0.addNote({60, 100, 0, 480});
    seq.addTrack(t0);
    MidiTrack t1;
    t1.setSolo(true);
    t1.addNote({64, 100, 0, 480});
    seq.addTrack(t1);

    const auto snap = PlaybackSnapshot::build(seq);
    REQUIRE(snap.notes.size() == 1);
    CHECK(snap.notes[0].ctx.trackId == seq.getTrack(1).getId());
}

TEST_CASE("mute wins over solo on the same track", "[engine][snapshot]")
{
    MidiSequence seq;
    MidiTrack t0;
    t0.addNote({60, 100, 0, 480});
    seq.addTrack(t0);
    MidiTrack t1;
    t1.setMuted(true);
    t1.setSolo(true);
    t1.addNote({64, 100, 0, 480});
    seq.addTrack(t1);
    MidiTrack t2;
    t2.setSolo(true);
    t2.addNote({67, 100, 0, 480});
    seq.addTrack(t2);

    const auto snap = PlaybackSnapshot::build(seq);
    REQUIRE(snap.notes.size() == 1);
    CHECK(snap.notes[0].ctx.trackId == seq.getTrack(2).getId());
}

TEST_CASE("without solo only muted tracks are inaudible", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.setTrackMuted(1, true);
    CHECK(isTrackAudible(seq, 0));
    CHECK_FALSE(isTrackAudible(seq, 1));
}

TEST_CASE("with solo only unmuted solo tracks are audible", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.addTrack();
    seq.setTrackSolo(1, true);
    seq.setTrackSolo(2, true);
    seq.setTrackMuted(2, true);
    CHECK_FALSE(isTrackAudible(seq, 0));
    CHECK(isTrackAudible(seq, 1));
    CHECK_FALSE(isTrackAudible(seq, 2));
}

TEST_CASE("contexts list every audible track including ones without events", "[engine][snapshot]")
{
    MidiSequence seq;
    MidiTrack t0;
    t0.setChannel(3);
    t0.addNote({60, 100, 0, 480});
    seq.addTrack(t0);
    seq.addTrack(MidiTrack{});
    MidiTrack t2;
    t2.setMuted(true);
    seq.addTrack(t2);

    const auto snap = PlaybackSnapshot::build(seq);
    CHECK(snap.contexts ==
          std::vector<PlaybackTrackContext>{makePlaybackTrackContext(seq, 0), makePlaybackTrackContext(seq, 1)});
}

TEST_CASE("routeTarget carries the id of an existing target track", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.setTrackRouteTarget(0, seq.getTrack(1).getId());
    seq.addNote(0, {60, 100, 0, 480});

    const auto snap = PlaybackSnapshot::build(seq);
    REQUIRE(snap.notes.size() == 1);
    CHECK(snap.notes[0].ctx.routeTarget == seq.getTrack(1).getId());
}

TEST_CASE("routeTarget falls back to own track when the target was removed", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.setTrackRouteTarget(0, seq.getTrack(1).getId());
    seq.removeTrack(1);
    seq.addNote(0, {60, 100, 0, 480});

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

TEST_CASE("tickAfter advances at the tempo of the tick", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 125.0}});

    const auto snap = PlaybackSnapshot::build(seq);
    CHECK_THAT(snap.tickAfter(100.0, 10.0), WithinAbs(110.0, 1e-9));
}

TEST_CASE("tickAfter defaults to 120 when there are no tempo changes", "[engine][snapshot]")
{
    const PlaybackSnapshot snap;
    CHECK_THAT(snap.tickAfter(0.0, 1000.0), WithinAbs(960.0, 1e-9));
}

TEST_CASE("tickAfter switches tempo at a change it crosses", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 125.0}, {100, 250.0}});

    const auto snap = PlaybackSnapshot::build(seq);
    CHECK_THAT(snap.tickAfter(90.0, 20.0), WithinAbs(120.0, 1e-9));
    CHECK_THAT(snap.tickAfter(99.5, 1.0), WithinAbs(101.0, 1e-9));
}

TEST_CASE("tickAfter crosses several tempo changes", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 125.0}, {100, 250.0}, {120, 125.0}});

    const auto snap = PlaybackSnapshot::build(seq);
    CHECK_THAT(snap.tickAfter(90.0, 30.0), WithinAbs(130.0, 1e-9));
}

TEST_CASE("tickAfter uses the new tempo from a change it starts on", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 125.0}, {100, 250.0}});

    const auto snap = PlaybackSnapshot::build(seq);
    CHECK_THAT(snap.tickAfter(100.0, 5.0), WithinAbs(110.0, 1e-9));
}

TEST_CASE("tickAfter stops on a change it reaches exactly", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 125.0}, {100, 250.0}});

    const auto snap = PlaybackSnapshot::build(seq);
    CHECK_THAT(snap.tickAfter(90.0, 10.0), WithinAbs(100.0, 1e-9));
}

TEST_CASE("msBetween measures at the tempo of the ticks", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 125.0}});

    const auto snap = PlaybackSnapshot::build(seq);
    CHECK_THAT(snap.msBetween(100.0, 110.0), WithinAbs(10.0, 1e-9));
}

TEST_CASE("msBetween defaults to 120 when there are no tempo changes", "[engine][snapshot]")
{
    const PlaybackSnapshot snap;
    CHECK_THAT(snap.msBetween(0.0, 960.0), WithinAbs(1000.0, 1e-9));
}

TEST_CASE("msBetween switches tempo at changes between the ticks", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 125.0}, {100, 250.0}, {120, 125.0}});

    const auto snap = PlaybackSnapshot::build(seq);
    CHECK_THAT(snap.msBetween(90.0, 130.0), WithinAbs(30.0, 1e-9));
    CHECK_THAT(snap.msBetween(99.5, 101.0), WithinAbs(1.0, 1e-9));
}

TEST_CASE("msBetween ignores a change at the end tick", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 125.0}, {100, 250.0}});

    const auto snap = PlaybackSnapshot::build(seq);
    CHECK_THAT(snap.msBetween(90.0, 100.0), WithinAbs(10.0, 1e-9));
    CHECK_THAT(snap.msBetween(100.0, 110.0), WithinAbs(5.0, 1e-9));
}

TEST_CASE("time signature changes are copied into the snapshot", "[engine][snapshot]")
{
    MidiSequence seq;
    TimeSignatureTest::setTimeSignatures(seq, {{1, 4, 4}, {3, 3, 4}});

    const auto snap = PlaybackSnapshot::build(seq);
    CHECK(snap.timeSignatureChanges == TimeSignatureTest::timeSigs({{1, 4, 4}, {3, 3, 4}}));
}

TEST_CASE("recorded notes and events are played on their track", "[engine][snapshot]")
{
    MidiSequence seq;
    MidiTrack t0;
    t0.setChannel(3);
    t0.addNote({60, 100, 0, 480});
    seq.addTrack(t0);
    const Take take{
        {seq.getTrack(0).getId(),
         {{{64, 90, 240, 240}}, {{.type = MidiEvent::Type::ControlChange, .tick = 120, .data1 = 7, .data2 = 90}}}}};

    const auto snap = PlaybackSnapshot::build(seq, &take);
    REQUIRE(snap.notes.size() == 2);
    CHECK(snap.notes[1].note.noteNumber == 64);
    CHECK(snap.notes[1].ctx.channel == 3);
    REQUIRE(snap.events.size() == 1);
    CHECK(snap.events[0].ctx.trackId == seq.getTrack(0).getId());
}

TEST_CASE("recorded notes on a muted track are not played", "[engine][snapshot]")
{
    MidiSequence seq;
    MidiTrack t0;
    t0.setMuted(true);
    seq.addTrack(t0);
    const Take take{{seq.getTrack(0).getId(), {{{64, 90, 240, 240}}, {}}}};

    CHECK(PlaybackSnapshot::build(seq, &take).notes.empty());
}

TEST_CASE("recorded notes for a track that is gone are not played", "[engine][snapshot]")
{
    MidiSequence seq;
    seq.addTrack();
    const Take take{{TrackId{999}, {{{64, 90, 240, 240}}, {}}}};

    CHECK(PlaybackSnapshot::build(seq, &take).notes.empty());
}

TEST_CASE("ramps on a track are filled in the snapshot", "[engine][snapshot][ramp]")
{
    MidiSequence seq;
    MidiTrack t0;
    t0.addEvent({.type = MidiEvent::Type::ControlChange, .tick = 0, .data1 = 1, .data2 = 0});
    t0.addEvent({.type = MidiEvent::Type::ControlChange, .tick = 4, .data1 = 1, .data2 = 4, .ramp = true});
    seq.addTrack(t0);

    const auto snap = PlaybackSnapshot::build(seq);
    REQUIRE(snap.events.size() == 5);
    for (int i = 0; i < 5; ++i)
        CHECK(snap.events[static_cast<size_t>(i)].event ==
              MidiEvent{.type = MidiEvent::Type::ControlChange, .tick = i, .data1 = 1, .data2 = i});
}

TEST_CASE("events of the recording take do not start a ramp", "[engine][snapshot][ramp]")
{
    MidiSequence seq;
    MidiTrack t0;
    t0.addEvent({.type = MidiEvent::Type::ControlChange, .tick = 4, .data1 = 1, .data2 = 4, .ramp = true});
    seq.addTrack(t0);
    const Take take{
        {seq.getTrack(0).getId(), {{}, {{.type = MidiEvent::Type::ControlChange, .tick = 0, .data1 = 1, .data2 = 0}}}}};

    const auto snap = PlaybackSnapshot::build(seq, &take);
    REQUIRE(snap.events.size() == 2);
    CHECK(snap.events[0].event == MidiEvent{.type = MidiEvent::Type::ControlChange, .tick = 0, .data1 = 1, .data2 = 0});
    CHECK(snap.events[1].event == MidiEvent{.type = MidiEvent::Type::ControlChange, .tick = 4, .data1 = 1, .data2 = 4});
}

TEST_CASE("a start inside a ramp sends the value just before the start", "[engine][snapshot][ramp]")
{
    MidiSequence seq;
    MidiTrack t0;
    t0.addEvent({.type = MidiEvent::Type::ControlChange, .tick = 0, .data1 = 1, .data2 = 0});
    t0.addEvent({.type = MidiEvent::Type::ControlChange, .tick = 100, .data1 = 1, .data2 = 100, .ramp = true});
    seq.addTrack(t0);

    const auto chased = chaseEvents(PlaybackSnapshot::build(seq), 50);
    REQUIRE(chased.size() == 1);
    CHECK(chased[0].event == MidiEvent{.type = MidiEvent::Type::ControlChange, .tick = 50, .data1 = 1, .data2 = 49});
}

TEST_CASE("a start on a ramp point leaves the point to playback", "[engine][snapshot][ramp]")
{
    MidiSequence seq;
    MidiTrack t0;
    t0.addEvent({.type = MidiEvent::Type::ControlChange, .tick = 0, .data1 = 1, .data2 = 0});
    t0.addEvent({.type = MidiEvent::Type::ControlChange, .tick = 100, .data1 = 1, .data2 = 100, .ramp = true});
    seq.addTrack(t0);

    const auto chased = chaseEvents(PlaybackSnapshot::build(seq), 100);
    REQUIRE(chased.size() == 1);
    CHECK(chased[0].event == MidiEvent{.type = MidiEvent::Type::ControlChange, .tick = 100, .data1 = 1, .data2 = 99});
}
