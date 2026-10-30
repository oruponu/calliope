#include "model/notation/NoteName.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("a note number is named by its pitch class with sharps and its octave from C-1", "[model][note-name]")
{
    CHECK(NoteName::format(60) == "C4");
    CHECK(NoteName::format(61) == "C#4");
    CHECK(NoteName::format(71) == "B4");
    CHECK(NoteName::format(0) == "C-1");
    CHECK(NoteName::format(127) == "G9");
}
