#include "support/ChordTestHelpers.h"
#include "support/TimeSignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <utility>

using namespace chordtest;

namespace
{
using Span = std::pair<int, int>;
} // namespace

TEST_CASE("an empty track offers the whole bar", "[chord][add-span]")
{
    MidiSequence seq;
    CHECK(seq.chordAddSpanAt(0) == Span{0, 1920});
    CHECK(seq.chordAddSpanAt(2500) == Span{1920, 3840});
}

TEST_CASE("a gap before the first chord ends at that chord", "[chord][add-span]")
{
    MidiSequence seq;
    seq.setChordChanges(chords({{2400, "C"}, {3840, "N.C."}}));
    CHECK(seq.chordAddSpanAt(2000) == Span{1920, 2400});
}

TEST_CASE("a visible chord offers nothing", "[chord][add-span]")
{
    MidiSequence seq;
    seq.setChordChanges(chords({{2400, "C"}, {3840, "N.C."}}));
    CHECK(seq.chordAddSpanAt(2400) == Span{0, 0});
    CHECK(seq.chordAddSpanAt(3000) == Span{0, 0});
}

TEST_CASE("after the last no-chord the span starts at the later of it and the bar", "[chord][add-span]")
{
    MidiSequence seq;
    seq.setChordChanges(chords({{0, "C"}, {2400, "N.C."}}));
    CHECK(seq.chordAddSpanAt(3000) == Span{2400, 3840});
    CHECK(seq.chordAddSpanAt(5000) == Span{3840, 5760});
}

TEST_CASE("consecutive no-chords start from the nearest one", "[chord][add-span]")
{
    MidiSequence seq;
    seq.setChordChanges(chords({{0, "C"}, {1920, "N.C."}, {2400, "N.C."}, {3840, "F"}, {5760, "N.C."}}));
    CHECK(seq.chordAddSpanAt(2000) == Span{1920, 2400});
    CHECK(seq.chordAddSpanAt(2600) == Span{2400, 3840});
}

TEST_CASE("a no-chord in the middle of a bar is a boundary", "[chord][add-span]")
{
    MidiSequence seq;
    seq.setChordChanges(chords({{0, "C"}, {960, "N.C."}}));
    CHECK(seq.chordAddSpanAt(100) == Span{0, 0});
    CHECK(seq.chordAddSpanAt(1000) == Span{960, 1920});
}

TEST_CASE("the span follows a 3/4 bar", "[chord][add-span]")
{
    MidiSequence seq;
    timesigtest::setTimeSignatures(seq, {{1, 4, 4}, {2, 3, 4}});
    CHECK(seq.chordAddSpanAt(2000) == Span{1920, 3360});
}

TEST_CASE("a negative tick offers nothing", "[chord][add-span]")
{
    MidiSequence seq;
    CHECK(seq.chordAddSpanAt(-1) == Span{0, 0});
}
