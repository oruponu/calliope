#include "support/KeySignatureTestHelpers.h"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <utility>

namespace
{
const std::array<std::string, 15> majorNames{"Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C",
                                             "G",  "D",  "A",  "E",  "B",  "F#", "C#"};
const std::array<std::string, 15> minorNames{"Abm", "Ebm", "Bbm", "Fm",  "Cm",  "Gm",  "Dm", "Am",
                                             "Em",  "Bm",  "F#m", "C#m", "G#m", "D#m", "A#m"};

using Key = std::pair<int, bool>;
const Key rejected{99, false};

Key parse(const std::string& text)
{
    int sharpsOrFlats = 0;
    bool isMinor = false;
    if (!KeySignatureName::fromString(text, sharpsOrFlats, isMinor))
        return rejected;
    return {sharpsOrFlats, isMinor};
}
} // namespace

TEST_CASE("seven sharps or flats are normalized to the simpler spelling", "[notation][keysig]")
{
    CHECK(KeySignatureName::normalizeSharpsOrFlats(7) == -5);
    CHECK(KeySignatureName::normalizeSharpsOrFlats(-7) == 5);
}

TEST_CASE("other sharps or flats are unchanged", "[notation][keysig]")
{
    for (int sf = -6; sf <= 6; ++sf)
    {
        CAPTURE(sf);
        CHECK(KeySignatureName::normalizeSharpsOrFlats(sf) == sf);
    }
}

TEST_CASE("every key signature has a name", "[notation][keysig]")
{
    for (int sf = -7; sf <= 7; ++sf)
    {
        CAPTURE(sf);
        CHECK(KeySignatureName::toString(sf, false) == majorNames[static_cast<size_t>(sf + 7)]);
        CHECK(KeySignatureName::toString(sf, true) == minorNames[static_cast<size_t>(sf + 7)]);
    }
}

TEST_CASE("out-of-range key signatures have no name", "[notation][keysig]")
{
    CHECK(KeySignatureName::toString(8, false) == "--");
    CHECK(KeySignatureName::toString(-8, true) == "--");
}

TEST_CASE("canonical names are parsed", "[notation][keysig]")
{
    for (int sf = -7; sf <= 7; ++sf)
    {
        CAPTURE(sf);
        CHECK(parse(majorNames[static_cast<size_t>(sf + 7)]) == Key{sf, false});
        CHECK(parse(minorNames[static_cast<size_t>(sf + 7)]) == Key{sf, true});
    }
}

TEST_CASE("enharmonic aliases are accepted", "[notation][keysig]")
{
    CHECK(parse("D#") == Key{-3, false});
    CHECK(parse("G#") == Key{-4, false});
    CHECK(parse("A#") == Key{-2, false});
    CHECK(parse("Dbm") == Key{4, true});
    CHECK(parse("Gbm") == Key{3, true});
}

TEST_CASE("spaces and letter case are ignored", "[notation][keysig]")
{
    CHECK(parse(" f# m ") == Key{3, true});
    CHECK(parse("bB") == Key{-2, false});
    CHECK(parse("EBM") == Key{-6, true});
}

TEST_CASE("rare spellings are rejected", "[notation][keysig]")
{
    for (const std::string text : {"E#", "Fb", "B#", "Cbm", "Fbm", "E#m", "B#m", "", "H"})
    {
        CAPTURE(text);
        CHECK(parse(text) == rejected);
    }
}

TEST_CASE("key signature names round-trip", "[notation][keysig]")
{
    for (int sf = -7; sf <= 7; ++sf)
        for (bool isMinor : {false, true})
        {
            CAPTURE(sf, isMinor);
            CHECK(parse(KeySignatureName::toString(sf, isMinor)) == Key{sf, isMinor});
        }
}
