#include "support/KeySignatureTestHelpers.h"
#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("the tempo at a tick is the last change at or before it", "[timeline][lookup]")
{
    MidiSequence seq;
    seq.setTempoChanges({{0, 120.0}, {1920, 90.0}, {3840, 150.0}});
    CHECK(seq.getTempoAt(1919) == 120.0);
    CHECK(seq.getTempoAt(1920) == 90.0);
    CHECK(seq.getTempoAt(3839) == 90.0);
    CHECK(seq.getTempoAt(3840) == 150.0);
    CHECK(seq.getTempoAt(100000) == 150.0);
    const auto change = seq.getTempoChangeAt(2000);
    CHECK(change.tick == 1920);
    CHECK(change.bpm == 90.0);
}

TEST_CASE("before the first tempo change the default tempo applies", "[timeline][lookup]")
{
    MidiSequence seq;
    seq.setTempoChanges({{480, 90.0}});
    CHECK(seq.getTempoAt(479) == 120.0);
    const auto before = seq.getTempoChangeAt(479);
    CHECK(before.tick == 0);
    CHECK(before.bpm == 120.0);

    seq.setTempoChanges({});
    CHECK(seq.getTempoAt(0) == 120.0);
    const auto empty = seq.getTempoChangeAt(0);
    CHECK(empty.tick == 0);
    CHECK(empty.bpm == 120.0);
}

TEST_CASE("a negative tick resolves to the initial tempo, time signature and key", "[timeline][lookup]")
{
    MidiSequence seq;
    CHECK(seq.getTempoAt(-1) == 120.0);
    const auto change = seq.getTempoChangeAt(-1);
    CHECK(change.tick == 0);
    CHECK(change.bpm == 120.0);
    CHECK(seq.getTimeSignatureAt(-1) == TimeSignatureChange{0, 4, 4});
    CHECK(seq.getKeySignatureAt(-1) == KeySignatureChange{0, 0, false});
}

TEST_CASE("the time signature at a tick is the last change at or before it", "[timeline][lookup]")
{
    MidiSequence seq;
    timesigtest::setTimeSignatures(seq, {{1, 4, 4}, {5, 3, 4}});
    CHECK(seq.getTimeSignatureAt(7679) == TimeSignatureChange{0, 4, 4});
    CHECK(seq.getTimeSignatureAt(7680) == TimeSignatureChange{7680, 3, 4});
    CHECK(seq.getTimeSignatureAt(100000) == TimeSignatureChange{7680, 3, 4});
}

TEST_CASE("before the first time signature change 4/4 applies", "[timeline][lookup]")
{
    MidiSequence seq;
    seq.setTimeSignatureChanges({{1920, 3, 4}});
    CHECK(seq.getTimeSignatureAt(1919) == TimeSignatureChange{0, 4, 4});
    seq.setTimeSignatureChanges({});
    CHECK(seq.getTimeSignatureAt(0) == TimeSignatureChange{0, 4, 4});
}

TEST_CASE("the key at a tick is the last change at or before it", "[timeline][lookup]")
{
    MidiSequence seq;
    seq.setKeySignatureChanges({{0, 1, false}, {3840, -3, true}});
    CHECK(seq.getKeySignatureAt(3839) == KeySignatureChange{0, 1, false});
    CHECK(seq.getKeySignatureAt(3840) == KeySignatureChange{3840, -3, true});
}

TEST_CASE("without a key change C major applies", "[timeline][lookup]")
{
    MidiSequence seq;
    CHECK(seq.getKeySignatureAt(0) == KeySignatureChange{0, 0, false});
    seq.setKeySignatureChanges({{1920, 2, false}});
    CHECK(seq.getKeySignatureAt(1919) == KeySignatureChange{0, 0, false});
}
