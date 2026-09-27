#include "model/MidiSequence.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <utility>

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

TEST_CASE("ids are not reused after replaceContents", "[model][trackid]")
{
    MidiSequence seq;
    const TrackId before = seq.addTrack().getId();
    SequenceContents contents;
    contents.tracks.emplace_back();
    seq.replaceContents(std::move(contents));
    CHECK(static_cast<std::uint32_t>(seq.getTrack(0).getId()) > static_cast<std::uint32_t>(before));
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

TEST_CASE("addTrack assigns a fresh id even to a track that already has one", "[model][trackid]")
{
    MidiSequence seq;
    seq.addTrack();
    const MidiTrack copy = seq.getTrack(0);
    seq.addTrack(copy);
    CHECK(seq.getTrack(1).getId() != seq.getTrack(0).getId());
}
