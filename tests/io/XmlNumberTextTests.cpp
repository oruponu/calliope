#include "io/XmlNumberText.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <limits>
#include <string>

TEST_CASE("parseInt reads whole decimal ints", "[io][xmlnumber]")
{
    CHECK(XmlNumberText::parseInt("0") == 0);
    CHECK(XmlNumberText::parseInt("42") == 42);
    CHECK(XmlNumberText::parseInt("-7") == -7);
    CHECK(XmlNumberText::parseInt("2147483647") == std::numeric_limits<int>::max());
    CHECK(XmlNumberText::parseInt("-2147483648") == std::numeric_limits<int>::min());
}

TEST_CASE("parseInt rejects text that is not a whole decimal int", "[io][xmlnumber]")
{
    const auto text =
        GENERATE(as<std::string>{}, "", " 1", "1 ", "+1", "1.0", "1e3", "12abc", "0x10", "2147483648", "-2147483649");
    CAPTURE(text);
    CHECK_FALSE(XmlNumberText::parseInt(text));
}

TEST_CASE("parseFiniteDouble reads finite numbers", "[io][xmlnumber]")
{
    CHECK(XmlNumberText::parseFiniteDouble("120") == 120.0);
    CHECK(XmlNumberText::parseFiniteDouble("-0.5") == -0.5);
    CHECK(XmlNumberText::parseFiniteDouble("1e3") == 1000.0);
}

TEST_CASE("parseFiniteDouble rejects malformed or non-finite text", "[io][xmlnumber]")
{
    const auto text =
        GENERATE(as<std::string>{}, "", " 1", "+1", "1.5x", "abc", "inf", "-inf", "infinity", "nan", "1e400");
    CAPTURE(text);
    CHECK_FALSE(XmlNumberText::parseFiniteDouble(text));
}

TEST_CASE("formatDouble comes back as the same value", "[io][xmlnumber]")
{
    const double value = GENERATE(120.0, 0.1, 1.0 / 3.0, 133.33333333333334, 1e-7, 987654.321);
    CAPTURE(value);
    CHECK(XmlNumberText::parseFiniteDouble(XmlNumberText::formatDouble(value)) == value);
}

TEST_CASE("formatDouble writes whole numbers without a fraction", "[io][xmlnumber]")
{
    CHECK(XmlNumberText::formatDouble(120.0) == "120");
}
