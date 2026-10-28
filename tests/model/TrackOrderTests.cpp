#include "model/MidiSequence.h"
#include "model/TrackId.h"
#include "model/TrackOrder.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("moving a track down puts it at the gap", "[model][track]")
{
    CHECK(TrackOrder::moveTogether(4, {0}, 3) == std::vector{1, 2, 0, 3});
}

TEST_CASE("moving a track up puts it at the gap", "[model][track]")
{
    CHECK(TrackOrder::moveTogether(4, {3}, 1) == std::vector{0, 3, 1, 2});
}

TEST_CASE("moving a track to the end puts it last", "[model][track]")
{
    CHECK(TrackOrder::moveTogether(3, {0}, 3) == std::vector{1, 2, 0});
}

TEST_CASE("separate tracks end up together in their original order", "[model][track]")
{
    CHECK(TrackOrder::moveTogether(5, {0, 3}, 2) == std::vector{1, 0, 3, 2, 4});
}

TEST_CASE("a gap inside the moved tracks gathers them around it", "[model][track]")
{
    CHECK(TrackOrder::moveTogether(5, {1, 3}, 3) == std::vector{0, 2, 1, 3, 4});
}

TEST_CASE("a gap next to a track leaves the order unchanged", "[model][track]")
{
    CHECK(TrackOrder::isIdentity(TrackOrder::moveTogether(3, {1}, 1)));
    CHECK(TrackOrder::isIdentity(TrackOrder::moveTogether(3, {1}, 2)));
    CHECK_FALSE(TrackOrder::isIdentity(TrackOrder::moveTogether(3, {1}, 0)));
}

TEST_CASE("the inverse order tells where each track went", "[model][track]")
{
    CHECK(TrackOrder::inverse({1, 2, 0, 3}) == std::vector{2, 0, 1, 3});
}

TEST_CASE("reordering tracks moves them with their ids", "[model][track]")
{
    MidiSequence seq;
    const TrackId first = seq.addTrack().getId();
    const TrackId second = seq.addTrack().getId();
    const TrackId third = seq.addTrack().getId();

    seq.reorderTracks({2, 0, 1});
    CHECK(seq.indexOf(third) == 0);
    CHECK(seq.indexOf(first) == 1);
    CHECK(seq.indexOf(second) == 2);
}
