#include "model/notation/ChordSymbol.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

namespace
{
constexpr int none = ChordChange::none;

int parseRoot(const std::string& text)
{
    int root = -1;
    return ChordSymbol::rootFromString(text, root) ? root : -1;
}

int parseType(const std::string& text)
{
    int type = -1;
    return ChordSymbol::typeFromString(text, type) ? type : -1;
}
} // namespace

TEST_CASE("roots are encoded as accidental and note nibbles", "[notation][chord]")
{
    CHECK(ChordSymbol::rootToString(0x31) == "C");
    CHECK(ChordSymbol::rootToString(0x41) == "C#");
    CHECK(ChordSymbol::rootToString(0x23) == "Eb");
    CHECK(ChordSymbol::rootToString(0x01) == "Cbbb");
    CHECK(ChordSymbol::rootToString(0x67) == "B###");
}

TEST_CASE("invalid roots have no name", "[notation][chord]")
{
    CHECK(ChordSymbol::rootToString(none).empty());
    CHECK(ChordSymbol::rootToString(0x30).empty());
    CHECK(ChordSymbol::rootToString(0x38).empty());
    CHECK(ChordSymbol::rootToString(0x71).empty());
}

TEST_CASE("root names are parsed", "[notation][chord]")
{
    CHECK(parseRoot("C") == 0x31);
    CHECK(parseRoot("c#") == 0x41);
    CHECK(parseRoot(" e b ") == 0x23);
    CHECK(parseRoot("Bbbb") == 0x07);
    CHECK(parseRoot("F###") == 0x64);
}

TEST_CASE("invalid root names are rejected", "[notation][chord]")
{
    for (const std::string text : {"", "H", "C#b", "C####", "Cbbbb", "Cx"})
    {
        CAPTURE(text);
        CHECK(parseRoot(text) == -1);
    }
}

TEST_CASE("every root round-trips", "[notation][chord]")
{
    for (int accidental = 0; accidental <= 6; ++accidental)
        for (int note = 1; note <= 7; ++note)
        {
            const int root = (accidental << 4) | note;
            CAPTURE(root);
            CHECK(parseRoot(ChordSymbol::rootToString(root)) == root);
        }
}

TEST_CASE("chord types follow the XF table", "[notation][chord]")
{
    CHECK(ChordSymbol::typeToString(0).empty());
    CHECK(ChordSymbol::typeToString(1) == "6");
    CHECK(ChordSymbol::typeToString(2) == "maj7");
    CHECK(ChordSymbol::typeToString(3) == "maj7#11");
    CHECK(ChordSymbol::typeToString(6) == "6add9");
    CHECK(ChordSymbol::typeToString(10) == "m7");
    CHECK(ChordSymbol::typeToString(15) == "mmaj7");
    CHECK(ChordSymbol::typeToString(19) == "7");
    CHECK(ChordSymbol::typeToString(24) == "13");
    CHECK(ChordSymbol::typeToString(28) == "augmaj7");
    CHECK(ChordSymbol::typeToString(30) == "1+8");
    CHECK(ChordSymbol::typeToString(33) == "sus2");
}

TEST_CASE("no-chord and out-of-range types have no name", "[notation][chord]")
{
    CHECK(ChordSymbol::typeToString(ChordChange::typeCount).empty());
    CHECK(ChordSymbol::typeToString(-1).empty());
    CHECK(ChordSymbol::typeToString(35).empty());
}

TEST_CASE("type names are parsed", "[notation][chord]")
{
    CHECK(parseType("m7") == 10);
    CHECK(parseType("") == 0);
    CHECK(parseType(" 7 b 9 ") == 25);
    CHECK(parseType("M7") == -1);
    CHECK(parseType("maj7 add") == -1);
}

TEST_CASE("every type round-trips", "[notation][chord]")
{
    for (int type = 0; type < ChordChange::typeCount; ++type)
    {
        CAPTURE(type);
        CHECK(parseType(ChordSymbol::typeToString(type)) == type);
    }
}

TEST_CASE("chords are written as root, type and bass", "[notation][chord]")
{
    CHECK(ChordSymbol::toString({0, 0x31, 0, none, none}) == "C");
    CHECK(ChordSymbol::toString({0, 0x23, 10, none, none}) == "Ebm7");
    CHECK(ChordSymbol::toString({0, 0x31, 0, 0x35, 0}) == "C/G");
    CHECK(ChordSymbol::toString({0, 0x31, 2, 0x30, 0}) == "Cmaj7");
}

TEST_CASE("no-chord has three encodings", "[notation][chord]")
{
    CHECK(ChordSymbol::toString({0, none, 0, none, none}).empty());
    CHECK(ChordSymbol::toString({0, 0x30, 0, none, none}).empty());
    CHECK(ChordSymbol::toString({0, 0x31, ChordChange::typeCount, none, none}).empty());
}

TEST_CASE("a root with an invalid accidental is shown as a placeholder", "[notation][chord]")
{
    CHECK(ChordSymbol::toString({0, 0x71, 0, none, none}) == "--");
}

TEST_CASE("chords are split into root, type and bass", "[notation][chord]")
{
    const auto parts = ChordSymbol::toParts({0, 0x41, 27, 0x33, 0});
    CHECK(parts.root == "C#");
    CHECK(parts.type == "7#9");
    CHECK(parts.bass == "E");
}

TEST_CASE("a chord without bass has an empty bass part", "[notation][chord]")
{
    const auto parts = ChordSymbol::toParts({0, 0x31, 0, none, none});
    CHECK(parts.root == "C");
    CHECK(parts.type.empty());
    CHECK(parts.bass.empty());
}

TEST_CASE("no-chord has no parts", "[notation][chord]")
{
    const auto parts = ChordSymbol::toParts(ChordChange::noChord(0));
    CHECK(parts.root.empty());
    CHECK(parts.type.empty());
    CHECK(parts.bass.empty());
}

TEST_CASE("a root with an invalid accidental is split as a placeholder only", "[notation][chord]")
{
    const auto parts = ChordSymbol::toParts({0, 0x71, 10, 0x33, 0});
    CHECK(parts.root == "--");
    CHECK(parts.type.empty());
    CHECK(parts.bass.empty());
}
