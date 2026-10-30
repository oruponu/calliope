#include "ui/eventlist/LengthText.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("a length is read as a number of ticks", "[ui][event-list]")
{
    CHECK(LengthText::parse("480") == 480);
    CHECK(LengthText::parse(" 120 ") == 120);
}

TEST_CASE("a length is kept at one tick or more and below the largest", "[ui][event-list]")
{
    CHECK(LengthText::parse("0") == 1);
    CHECK(LengthText::parse("99999999999999999999") == LengthText::kMaxLength);
}

TEST_CASE("text that is not a number of ticks is not read as a length", "[ui][event-list]")
{
    CHECK_FALSE(LengthText::parse("").has_value());
    CHECK_FALSE(LengthText::parse("-5").has_value());
    CHECK_FALSE(LengthText::parse("1.5").has_value());
    CHECK_FALSE(LengthText::parse("4C").has_value());
}
