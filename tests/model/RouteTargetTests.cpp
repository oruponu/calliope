#include "model/MidiSequence.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("track without a route target resolves to itself", "[model][route]")
{
    MidiSequence seq;
    seq.addTrack();
    CHECK(seq.resolveRouteTarget(0) == seq.getTrack(0).getId());
}

TEST_CASE("route target that exists resolves to the target", "[model][route]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    const TrackId target = seq.getTrack(1).getId();
    seq.getTrack(0).setRouteTarget(target);
    CHECK(seq.resolveRouteTarget(0) == target);
}

TEST_CASE("route target that was removed resolves to the track itself", "[model][route]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    seq.getTrack(0).setRouteTarget(seq.getTrack(1).getId());
    seq.removeTrack(1);
    CHECK(seq.resolveRouteTarget(0) == seq.getTrack(0).getId());
}

TEST_CASE("route target restored by insertTrack resolves to the target again", "[model][route]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    const MidiTrack saved = seq.getTrack(1);
    seq.getTrack(0).setRouteTarget(saved.getId());
    seq.removeTrack(1);
    seq.insertTrack(1, saved);
    CHECK(seq.resolveRouteTarget(0) == saved.getId());
}

TEST_CASE("route target pointing to the track itself resolves to itself", "[model][route]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.getTrack(0).setRouteTarget(seq.getTrack(0).getId());
    CHECK(seq.resolveRouteTarget(0) == seq.getTrack(0).getId());
}
