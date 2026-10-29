#include "ui/eventlist/MetaEventDescription.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string>

TEST_CASE("text meta events show their name and text", "[ui][event-list][meta]")
{
    CHECK(MetaEventDescription::describe(0x06, {'I', 'n', 't', 'r', 'o'}) == "Marker Intro");
    CHECK(MetaEventDescription::describe(0x05, {0xE3, 0x81, 0x82}) == juce::String::fromUTF8("Lyric \xE3\x81\x82"));
}

TEST_CASE("each text meta event type has its name", "[ui][event-list][meta]")
{
    const auto [type, name] = GENERATE(table<int, std::string>({{0x01, "Text"},
                                                                {0x02, "Copyright"},
                                                                {0x03, "Track Name"},
                                                                {0x04, "Instrument"},
                                                                {0x05, "Lyric"},
                                                                {0x06, "Marker"},
                                                                {0x07, "Cue Point"},
                                                                {0x08, "Program Name"},
                                                                {0x09, "Device Name"}}));
    CAPTURE(type);
    CHECK(MetaEventDescription::describe(type, {'x'}) == juce::String(name) + " x");
}

TEST_CASE("control characters in meta text become spaces", "[ui][event-list][meta]")
{
    CHECK(MetaEventDescription::describe(0x05, {'a', 0x0D, 'b', 0x0A}) == "Lyric a b ");
    CHECK(MetaEventDescription::describe(0x05, {'a', 0x00, 'b'}) == "Lyric a b");
}

TEST_CASE("an empty text meta event shows only its name", "[ui][event-list][meta]")
{
    CHECK(MetaEventDescription::describe(0x05, {}) == "Lyric");
}

TEST_CASE("other meta events show their type and bytes in hex", "[ui][event-list][meta]")
{
    CHECK(MetaEventDescription::describe(0x21, {0x00}) == "Meta FF 21 00");
    CHECK(MetaEventDescription::describe(0x7F, {0x43, 0x7B, 0x00}) == "Meta FF 7F 43 7B 00");
    CHECK(MetaEventDescription::describe(0x00, {}) == "Meta FF 00");
}
