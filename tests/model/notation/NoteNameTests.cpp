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

TEST_CASE("a note number is read as itself", "[model][note-name]")
{
    CHECK(NoteName::parse("65") == 65);
    CHECK(NoteName::parse("0") == 0);
    CHECK(NoteName::parse(" 64 ") == 64);
}

TEST_CASE("a note name is read in either case with a sharp or a flat", "[model][note-name]")
{
    CHECK(NoteName::parse("C4") == 60);
    CHECK(NoteName::parse("c4") == 60);
    CHECK(NoteName::parse("C#4") == 61);
    CHECK(NoteName::parse("Db4") == 61);
    CHECK(NoteName::parse("bb3") == 58);
    CHECK(NoteName::parse("C-1") == 0);
    CHECK(NoteName::parse("G9") == 127);
}

TEST_CASE("a note outside the MIDI range is kept inside it", "[model][note-name]")
{
    CHECK(NoteName::parse("200") == 127);
    CHECK(NoteName::parse("99999999999999999999") == 127);
    CHECK(NoteName::parse("B9") == 127);
    CHECK(NoteName::parse("Cb-1") == 0);
    CHECK(NoteName::parse("C-5") == 0);
}

TEST_CASE("text that is neither a note number nor a note name is not read", "[model][note-name]")
{
    CHECK_FALSE(NoteName::parse("").has_value());
    CHECK_FALSE(NoteName::parse("H4").has_value());
    CHECK_FALSE(NoteName::parse("C").has_value());
    CHECK_FALSE(NoteName::parse("C#").has_value());
    CHECK_FALSE(NoteName::parse("4C").has_value());
    CHECK_FALSE(NoteName::parse("C4x").has_value());
    CHECK_FALSE(NoteName::parse("-5").has_value());
    CHECK_FALSE(NoteName::parse("C--1").has_value());
}
