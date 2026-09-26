#include "model/MidiSequence.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

TEST_CASE("addTrack assigns distinct increasing ids", "[model][trackid]")
{
    MidiSequence seq;
    const TrackId a = seq.addTrack().getId();
    const TrackId b = seq.addTrack().getId();
    CHECK(a != b);
    CHECK(static_cast<std::uint32_t>(a) < static_cast<std::uint32_t>(b));
}

TEST_CASE("track ids are never the default value", "[model][trackid]")
{
    MidiSequence seq;
    CHECK(seq.addTrack().getId() != TrackId{});
}

TEST_CASE("insertTrack keeps the id of the inserted track", "[model][trackid]")
{
    MidiSequence seq;
    seq.addTrack();
    seq.addTrack();
    const MidiTrack saved = seq.getTrack(0);
    seq.removeTrack(0);
    seq.insertTrack(0, saved);
    CHECK(seq.getTrack(0).getId() == saved.getId());
    CHECK(seq.indexOf(saved.getId()) == 0);
}

TEST_CASE("ids are not reused after clear", "[model][trackid]")
{
    MidiSequence seq;
    const TrackId before = seq.addTrack().getId();
    seq.clear();
    const TrackId after = seq.addTrack().getId();
    CHECK(static_cast<std::uint32_t>(after) > static_cast<std::uint32_t>(before));
}

TEST_CASE("ids are not reused after removeTrack", "[model][trackid]")
{
    MidiSequence seq;
    seq.addTrack();
    const TrackId removed = seq.addTrack().getId();
    seq.removeTrack(1);
    const TrackId added = seq.addTrack().getId();
    CHECK(added != removed);
}

TEST_CASE("indexOf follows tracks and returns -1 for unknown ids", "[model][trackid]")
{
    MidiSequence seq;
    const TrackId a = seq.addTrack().getId();
    seq.addTrack();
    const TrackId c = seq.addTrack().getId();
    CHECK(seq.indexOf(c) == 2);
    seq.removeTrack(0);
    CHECK(seq.indexOf(c) == 1);
    CHECK(seq.indexOf(a) == -1);
    CHECK(seq.indexOf(TrackId{}) == -1);
}
