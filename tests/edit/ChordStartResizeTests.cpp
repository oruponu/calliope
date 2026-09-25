#include "support/ChordTestHelpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace chordtest;

namespace
{
std::vector<ChordChange> gapped()
{
    return chords({{0, "A"}, {1920, "N.C."}, {3840, "C"}, {5760, "N.C."}});
}

std::vector<ChordChange> joined()
{
    return chords({{0, "A"}, {1920, "B"}, {3840, "C"}, {5760, "N.C."}});
}

std::vector<ChordChange> startResize(const std::vector<ChordChange>& before, int index, int targetStart)
{
    return ChordTrackEdits::afterStartResize(before, index, targetStart, grid);
}
} // namespace

TEST_CASE("3-1 left edge into the gap extends the chord", "[chord][start-resize]")
{
    CHECK(startResize(gapped(), 2, 2880) == chords({{0, "A"}, {1920, "N.C."}, {2880, "C"}, {5760, "N.C."}}));
}

TEST_CASE("3-2 left edge into the previous chord trims it", "[chord][start-resize]")
{
    CHECK(startResize(gapped(), 2, 960) == chords({{0, "A"}, {960, "C"}, {5760, "N.C."}}));
}

TEST_CASE("3-2 left edge before the previous chord removes it", "[chord][start-resize]")
{
    const auto expected = chords({{0, "C"}, {5760, "N.C."}});
    CHECK(startResize(gapped(), 2, 0) == expected);
    CHECK(startResize(gapped(), 2, -480) == expected);
}

TEST_CASE("3-3 left edge to the right shrinks the chord", "[chord][start-resize]")
{
    CHECK(startResize(gapped(), 2, 4800) == chords({{0, "A"}, {1920, "N.C."}, {4800, "C"}, {5760, "N.C."}}));
}

TEST_CASE("3-3 left edge to the right keeps at least one grid", "[chord][start-resize]")
{
    CHECK(startResize(gapped(), 2, 6000) == chords({{0, "A"}, {1920, "N.C."}, {5520, "C"}, {5760, "N.C."}}));
}

TEST_CASE("2-4 joined boundary to the left trims or removes the previous chord", "[chord][start-resize]")
{
    CHECK(startResize(joined(), 1, 960) == chords({{0, "A"}, {960, "B"}, {3840, "C"}, {5760, "N.C."}}));
    CHECK(startResize(joined(), 1, 0) == chords({{0, "B"}, {3840, "C"}, {5760, "N.C."}}));
}

TEST_CASE("open-ended last chord has no upper limit", "[chord][start-resize]")
{
    const auto before = chords({{0, "A"}, {1920, "B"}});
    CHECK(startResize(before, 1, 4800) == chords({{0, "A"}, {4800, "B"}}));
}

TEST_CASE("off-grid chord dragged right never moves its start left", "[chord][start-resize]")
{
    const auto before = chords({{0, "A"}, {1920, "N.C."}, {3850, "C"}, {5760, "N.C."}});
    CHECK(startResize(before, 2, 3900) == chords({{0, "A"}, {1920, "N.C."}, {4080, "C"}, {5760, "N.C."}}));

    const auto shortChord = chords({{0, "A"}, {1920, "N.C."}, {3850, "C"}, {4000, "N.C."}});
    CHECK(startResize(shortChord, 2, 3900) == shortChord);
}

TEST_CASE("off-grid chord dragged left never moves its start right", "[chord][start-resize]")
{
    const auto before = chords({{0, "A"}, {1920, "N.C."}, {3830, "C"}, {5760, "N.C."}});
    CHECK(startResize(before, 2, 3820) == chords({{0, "A"}, {1920, "N.C."}, {3600, "C"}, {5760, "N.C."}}));
}

TEST_CASE("less than half a grid is a no-op", "[chord][start-resize]")
{
    const auto before = gapped();
    CHECK(startResize(before, 2, 3830) == before);
    CHECK(startResize(before, 2, 3900) == before);
}

TEST_CASE("start resize is a no-op in degenerate cases", "[chord][start-resize]")
{
    const auto before = gapped();
    CHECK(startResize(before, 2, 3840) == before);
    CHECK(startResize(before, -1, 2880) == before);
    CHECK(startResize(before, 4, 2880) == before);
    CHECK(startResize(before, 1, 960) == before);
    CHECK(ChordTrackEdits::afterStartResize(before, 2, 2880, 0) == before);
}
