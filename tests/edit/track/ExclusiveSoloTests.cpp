#include "edit/track/ExclusiveSolo.h"
#include "model/MidiSequence.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
void addTracks(MidiSequence& seq, int count)
{
    for (int i = 0; i < count; ++i)
        seq.addTrack();
}
} // namespace

TEST_CASE("exclusive solo unsolos the other tracks", "[model][track]")
{
    MidiSequence seq;
    addTracks(seq, 3);
    seq.setTrackSolo(0, true);
    seq.setTrackSolo(2, true);

    ExclusiveSolo::toggle(seq, {1});
    CHECK_FALSE(seq.getTrack(0).isSolo());
    CHECK(seq.getTrack(1).isSolo());
    CHECK_FALSE(seq.getTrack(2).isSolo());
}

TEST_CASE("exclusive solo on a soloed track among others keeps only that track", "[model][track]")
{
    MidiSequence seq;
    addTracks(seq, 2);
    seq.setTrackSolo(0, true);
    seq.setTrackSolo(1, true);

    ExclusiveSolo::toggle(seq, {0});
    CHECK(seq.getTrack(0).isSolo());
    CHECK_FALSE(seq.getTrack(1).isSolo());
}

TEST_CASE("exclusive solo on the only soloed tracks unsolos them", "[model][track]")
{
    MidiSequence seq;
    addTracks(seq, 3);
    seq.setTrackSolo(0, true);
    seq.setTrackSolo(1, true);

    ExclusiveSolo::toggle(seq, {0, 1});
    CHECK_FALSE(seq.isAnySolo());
}

TEST_CASE("exclusive solo solos several tracks together", "[model][track]")
{
    MidiSequence seq;
    addTracks(seq, 3);

    ExclusiveSolo::toggle(seq, {0, 2});
    CHECK(seq.getTrack(0).isSolo());
    CHECK_FALSE(seq.getTrack(1).isSolo());
    CHECK(seq.getTrack(2).isSolo());
}
