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

std::vector<ChordChange> resize(const std::vector<ChordChange>& before, int index, int targetEnd)
{
    return ChordTrackEdits::afterResize(before, index, targetEnd, grid);
}
} // namespace

TEST_CASE("1-1 right edge into the middle of the gap", "[chord][resize]")
{
    CHECK(resize(gapped(), 0, 2880) == chords({{0, "A"}, {2880, "N.C."}, {3840, "C"}, {5760, "N.C."}}));
}

TEST_CASE("1-2 right edge exactly to the next chord joins them", "[chord][resize]")
{
    CHECK(resize(gapped(), 0, 3840) == chords({{0, "A"}, {3840, "C"}, {5760, "N.C."}}));
}

TEST_CASE("1-3 right edge into the middle of the next chord keeps its tail", "[chord][resize]")
{
    CHECK(resize(gapped(), 0, 4800) == chords({{0, "A"}, {4800, "C"}, {5760, "N.C."}}));
}

TEST_CASE("1-4 right edge past the end of the next chord removes it", "[chord][resize]")
{
    CHECK(resize(gapped(), 0, 6720) == chords({{0, "A"}, {6720, "N.C."}}));
}

TEST_CASE("1-5 right edge to the left moves the terminator", "[chord][resize]")
{
    const auto before = chords({{0, "A"}, {1920, "N.C."}});
    CHECK(resize(before, 0, 960) == chords({{0, "A"}, {960, "N.C."}}));
}

TEST_CASE("1-6 right edge before the start keeps at least one grid", "[chord][resize]")
{
    const auto before = chords({{960, "A"}, {2880, "N.C."}});
    CHECK(resize(before, 0, 0) == chords({{960, "A"}, {1200, "N.C."}}));
}

TEST_CASE("1-7 open-ended last chord gets a terminator", "[chord][resize]")
{
    const auto before = chords({{0, "A"}, {1920, "B"}});
    CHECK(resize(before, 1, 4800) == chords({{0, "A"}, {1920, "B"}, {4800, "N.C."}}));
    CHECK(resize(before, 1, 2400) == chords({{0, "A"}, {1920, "B"}, {2400, "N.C."}}));
}

TEST_CASE("1-8 non-canonical terminator keeps its fields", "[chord][resize]")
{
    const std::vector<ChordChange> before{chord(0, "A"), xfNoChord(1920), chord(3840, "C"), chord(5760, "N.C.")};

    const std::vector<ChordChange> extended{chord(0, "A"), xfNoChord(2880), chord(3840, "C"), chord(5760, "N.C.")};
    CHECK(resize(before, 0, 2880) == extended);

    const std::vector<ChordChange> shrunk{chord(0, "A"), xfNoChord(960), chord(3840, "C"), chord(5760, "N.C.")};
    CHECK(resize(before, 0, 960) == shrunk);
}

TEST_CASE("1-9 redundant no-chords are consumed and the result is normalized", "[chord][resize]")
{
    const auto before = chords({{0, "A"}, {1920, "N.C."}, {2400, "N.C."}, {3840, "C"}, {5760, "N.C."}});
    CHECK(resize(before, 0, 2880) == chords({{0, "A"}, {2880, "N.C."}, {3840, "C"}, {5760, "N.C."}}));
}

TEST_CASE("2-1 joined boundary to the right keeps the tail of the next chord", "[chord][resize]")
{
    CHECK(resize(joined(), 0, 2880) == chords({{0, "A"}, {2880, "B"}, {3840, "C"}, {5760, "N.C."}}));
}

TEST_CASE("2-2 joined boundary exactly to the chord after next removes the middle one", "[chord][resize]")
{
    CHECK(resize(joined(), 0, 3840) == chords({{0, "A"}, {3840, "C"}, {5760, "N.C."}}));
}

TEST_CASE("2-3 joined boundary into the chord after next", "[chord][resize]")
{
    CHECK(resize(joined(), 0, 4800) == chords({{0, "A"}, {4800, "C"}, {5760, "N.C."}}));
}

TEST_CASE("joined boundary to the left rolls the next chord", "[chord][resize]")
{
    CHECK(resize(joined(), 0, 960) == chords({{0, "A"}, {960, "B"}, {3840, "C"}, {5760, "N.C."}}));
}

TEST_CASE("end snaps to the nearest grid", "[chord][resize]")
{
    CHECK(resize(gapped(), 0, 2890) == chords({{0, "A"}, {2880, "N.C."}, {3840, "C"}, {5760, "N.C."}}));
    CHECK(resize(gapped(), 0, 3010) == chords({{0, "A"}, {3120, "N.C."}, {3840, "C"}, {5760, "N.C."}}));
    CHECK(resize(gapped(), 0, 3000) == chords({{0, "A"}, {3120, "N.C."}, {3840, "C"}, {5760, "N.C."}}));
}

TEST_CASE("resize is a no-op in degenerate cases", "[chord][resize]")
{
    const auto before = gapped();
    CHECK(resize(before, 0, 1920) == before);
    CHECK(resize(before, 0, 2000) == before);
    CHECK(resize(before, -1, 2880) == before);
    CHECK(resize(before, 4, 2880) == before);
    CHECK(resize(before, 1, 2880) == before);
    CHECK(ChordTrackEdits::afterResize(before, 0, 2880, 0) == before);
}
