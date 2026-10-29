#include "model/notation/SysExHex.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstdint>
#include <string>
#include <vector>

TEST_CASE("sysex bytes are formatted as spaced uppercase hex", "[model][sysex]")
{
    CHECK(SysExHex::format({0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41}) == "41 10 42 12 40 00 7F 00 41");
    CHECK(SysExHex::format({0x0A}) == "0A");
}

TEST_CASE("formatted sysex bytes parse back", "[model][sysex]")
{
    CHECK(SysExHex::parse("41 10 42 12 40 00 7F 00 41") ==
          std::vector<std::uint8_t>{0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41});
    CHECK(SysExHex::parse("FF") == std::vector<std::uint8_t>{0xFF});
}

TEST_CASE("text that format would not produce does not parse", "[model][sysex]")
{
    const auto text =
        GENERATE(as<std::string>{}, "", " ", "41  10", " 41", "41 ", "4", "411", "4G", "7f", "41,10", "41 1", "41\t10");
    CAPTURE(text);
    CHECK_FALSE(SysExHex::parse(text));
}
