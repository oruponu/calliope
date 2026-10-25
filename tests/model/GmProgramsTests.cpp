#include "model/GmPrograms.h"
#include <catch2/catch_test_macros.hpp>
#include <string_view>

TEST_CASE("GM programs are named by number", "[model][program-change]")
{
    CHECK(std::string_view(GmPrograms::programName(0)) == "Acoustic Grand Piano");
    CHECK(std::string_view(GmPrograms::programName(40)) == "Violin");
    CHECK(std::string_view(GmPrograms::programName(127)) == "Gunshot");
    CHECK(GmPrograms::programName(128) == nullptr);
    CHECK(GmPrograms::programName(-1) == nullptr);
}
