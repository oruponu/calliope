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
    TimelineMap timeline;
    CHECK(ChordTrackEdits::addSpanAt({}, 0, timeline) == Span{0, 1920});
    CHECK(ChordTrackEdits::addSpanAt({}, 2500, timeline) == Span{1920, 3840});
}

TEST_CASE("a gap before the first chord ends at that chord", "[chord][add-span]")
{
    TimelineMap timeline;
    const auto existing = chords({{2400, "C"}, {3840, "N.C."}});
    CHECK(ChordTrackEdits::addSpanAt(existing, 2000, timeline) == Span{1920, 2400});
}

TEST_CASE("a visible chord offers nothing", "[chord][add-span]")
{
    TimelineMap timeline;
    const auto existing = chords({{2400, "C"}, {3840, "N.C."}});
    CHECK(ChordTrackEdits::addSpanAt(existing, 2400, timeline) == Span{0, 0});
    CHECK(ChordTrackEdits::addSpanAt(existing, 3000, timeline) == Span{0, 0});
}

TEST_CASE("after the last no-chord the span starts at the later of it and the bar", "[chord][add-span]")
{
    TimelineMap timeline;
    const auto existing = chords({{0, "C"}, {2400, "N.C."}});
    CHECK(ChordTrackEdits::addSpanAt(existing, 3000, timeline) == Span{2400, 3840});
    CHECK(ChordTrackEdits::addSpanAt(existing, 5000, timeline) == Span{3840, 5760});
}

TEST_CASE("consecutive no-chords start from the nearest one", "[chord][add-span]")
{
    TimelineMap timeline;
    const auto existing = chords({{0, "C"}, {1920, "N.C."}, {2400, "N.C."}, {3840, "F"}, {5760, "N.C."}});
    CHECK(ChordTrackEdits::addSpanAt(existing, 2000, timeline) == Span{1920, 2400});
    CHECK(ChordTrackEdits::addSpanAt(existing, 2600, timeline) == Span{2400, 3840});
}

TEST_CASE("a no-chord in the middle of a bar is a boundary", "[chord][add-span]")
{
    TimelineMap timeline;
    const auto existing = chords({{0, "C"}, {960, "N.C."}});
    CHECK(ChordTrackEdits::addSpanAt(existing, 100, timeline) == Span{0, 0});
    CHECK(ChordTrackEdits::addSpanAt(existing, 1000, timeline) == Span{960, 1920});
}

TEST_CASE("the span follows a 3/4 bar", "[chord][add-span]")
{
    TimelineMap timeline;
    timesigtest::setTimeSignatures(timeline, {{1, 4, 4}, {2, 3, 4}});
    CHECK(ChordTrackEdits::addSpanAt({}, 2000, timeline) == Span{1920, 3360});
}

TEST_CASE("a negative tick offers nothing", "[chord][add-span]")
{
    TimelineMap timeline;
    CHECK(ChordTrackEdits::addSpanAt({}, -1, timeline) == Span{0, 0});
}
