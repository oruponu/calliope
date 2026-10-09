#include "engine/ThruTarget.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("index outside the track list has no thru target", "[engine][thru]")
{
    MidiSequence seq;
    seq.addTrack();
    CHECK_FALSE(resolveThruTarget(seq, -1).has_value());
    CHECK_FALSE(resolveThruTarget(seq, 1).has_value());
}

TEST_CASE("track with no output has no thru target", "[engine][thru]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTrackOutputDestination(0, MidiTrack::OutputDestination::None);
    CHECK_FALSE(resolveThruTarget(seq, 0).has_value());
}

TEST_CASE("muted track has no thru target", "[engine][thru]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.setTrackMuted(0, true);
    CHECK_FALSE(resolveThruTarget(seq, 0).has_value());
}

TEST_CASE("track silenced by another track's solo has no thru target", "[engine][thru]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.setTrackSolo(1, true);
    CHECK_FALSE(resolveThruTarget(seq, 0).has_value());
}

TEST_CASE("soloed active track keeps its thru target", "[engine][thru]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.setTrackSolo(0, true);
    const auto target = resolveThruTarget(seq, 0);
    REQUIRE(target.has_value());
    CHECK(target->trackId == seq.getTrack(0).getId());
}

TEST_CASE("thru target carries the track's channel and output", "[engine][thru]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.setTrackChannel(1, 4);
    const auto target = resolveThruTarget(seq, 1);
    REQUIRE(target.has_value());
    CHECK(target->trackId == seq.getTrack(1).getId());
    CHECK(target->channel == 4);
    CHECK(target->routeTarget == seq.getTrack(1).getId());
    CHECK(target->destination == MidiTrack::OutputDestination::MidiDevice);
}

TEST_CASE("routed track sends to the route target on its own channel", "[engine][thru]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.setTrackOutputDestination(0, MidiTrack::OutputDestination::Plugin);
    seq.setTrackChannel(0, 3);
    seq.setTrackRouteTarget(0, seq.getTrack(1).getId());
    const auto target = resolveThruTarget(seq, 0);
    REQUIRE(target.has_value());
    CHECK(target->trackId == seq.getTrack(0).getId());
    CHECK(target->channel == 3);
    CHECK(target->routeTarget == seq.getTrack(1).getId());
    CHECK(target->destination == MidiTrack::OutputDestination::Plugin);
}
