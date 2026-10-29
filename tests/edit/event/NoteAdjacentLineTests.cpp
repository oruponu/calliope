#include "edit/event/NoteEdits.h"
#include "support/GridTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <limits>

using namespace GridTest;

TEST_CASE("moving forward from the last line of a bar stops at the bar line", "[note][adjacent-line]")
{
    CHECK(NoteEdits::deltaToAdjacentLine(1280, true, threeFourQuarterTriplets) == 160);
}

TEST_CASE("moving back from a bar line stops at the last line of the previous bar", "[note][adjacent-line]")
{
    CHECK(NoteEdits::deltaToAdjacentLine(1440, false, threeFourQuarterTriplets) == -160);
}

TEST_CASE("moving forward inside a bar goes a full step", "[note][adjacent-line]")
{
    CHECK(NoteEdits::deltaToAdjacentLine(640, true, threeFourQuarterTriplets) == 320);
}

TEST_CASE("an off-grid tick moves onto the nearest line in the direction", "[note][adjacent-line]")
{
    CHECK(NoteEdits::deltaToAdjacentLine(1300, true, fourFourQuarters) == 140);
    CHECK(NoteEdits::deltaToAdjacentLine(1300, false, fourFourQuarters) == -340);
}

TEST_CASE("there is no move when the adjacent line is outside int", "[note][adjacent-line]")
{
    CHECK_FALSE(NoteEdits::deltaToAdjacentLine(std::numeric_limits<int>::max(), true, fourFourQuarters).has_value());
    CHECK_FALSE(NoteEdits::deltaToAdjacentLine(std::numeric_limits<int>::min(), false, fourFourQuarters).has_value());
}
