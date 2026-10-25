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

TEST_CASE("GM families are named in order", "[model][program-change]")
{
    CHECK(std::string_view(GmPrograms::familyName(0)) == "Piano");
    CHECK(std::string_view(GmPrograms::familyName(5)) == "Strings");
    CHECK(std::string_view(GmPrograms::familyName(15)) == "Sound Effects");
    CHECK(GmPrograms::familyName(16) == nullptr);
}
