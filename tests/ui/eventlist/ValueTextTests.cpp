#include "ui/eventlist/ValueText.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("a value is read as a whole number, with a minus sign below zero", "[ui][event-list]")
{
    CHECK(ValueText::parse("100", 0, 127) == 100);
    CHECK(ValueText::parse(" 64 ", 0, 127) == 64);
    CHECK(ValueText::parse("-8192", -8192, 8191) == -8192);
}

TEST_CASE("a value is kept inside its range", "[ui][event-list]")
{
    CHECK(ValueText::parse("200", 0, 127) == 127);
    CHECK(ValueText::parse("0", 1, 127) == 1);
    CHECK(ValueText::parse("-5", 0, 127) == 0);
    CHECK(ValueText::parse("-99999999999999999999", -8192, 8191) == -8192);
}

TEST_CASE("text that is not a whole number is not read as a value", "[ui][event-list]")
{
    CHECK_FALSE(ValueText::parse("", 0, 127).has_value());
    CHECK_FALSE(ValueText::parse("-", 0, 127).has_value());
    CHECK_FALSE(ValueText::parse("1.5", 0, 127).has_value());
    CHECK_FALSE(ValueText::parse("5-", 0, 127).has_value());
}
