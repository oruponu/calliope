#include "notation/ChordSymbol.h"
#include <array>
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

std::string spell(int semitone, ChordSpelling spelling)
{
    return ChordSymbol::rootToString(ChordSymbol::semitoneToRoot(semitone, spelling));
}

const std::array<std::string, 12> sharpNames{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
const std::array<std::string, 12> flatNames{"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
const std::array<std::string, 12> mixedNames{"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
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
    CHECK(ChordSymbol::typeToString(2) == "M7");
    CHECK(ChordSymbol::typeToString(10) == "m7");
    CHECK(ChordSymbol::typeToString(19) == "7");
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
    CHECK(parseType(" 7 ( 9 ) ") == 22);
    CHECK(parseType("maj7") == -1);
    CHECK(parseType("M7 add") == -1);
}

TEST_CASE("every type round-trips", "[notation][chord]")
{
    for (int type = 0; type < ChordChange::typeCount; ++type)
    {
        CAPTURE(type);
        CHECK(parseType(ChordSymbol::typeToString(type)) == type);
    }
}

TEST_CASE("roots map to semitones", "[notation][chord]")
{
    CHECK(ChordSymbol::rootToSemitone(0x31) == 0);
    CHECK(ChordSymbol::rootToSemitone(0x41) == 1);
    CHECK(ChordSymbol::rootToSemitone(0x21) == 11);
    CHECK(ChordSymbol::rootToSemitone(0x37) == 11);
    CHECK(ChordSymbol::rootToSemitone(0x01) == 9);
    CHECK(ChordSymbol::rootToSemitone(0x07) == 8);
    CHECK(ChordSymbol::rootToSemitone(0x67) == 2);
}

TEST_CASE("invalid roots have no semitone", "[notation][chord]")
{
    CHECK(ChordSymbol::rootToSemitone(none) == -1);
    CHECK(ChordSymbol::rootToSemitone(0x30) == -1);
}

TEST_CASE("semitones are spelled by the chosen spelling", "[notation][chord]")
{
    for (int semitone = 0; semitone < 12; ++semitone)
    {
        CAPTURE(semitone);
        CHECK(spell(semitone, ChordSpelling::Sharp) == sharpNames[static_cast<size_t>(semitone)]);
        CHECK(spell(semitone, ChordSpelling::Flat) == flatNames[static_cast<size_t>(semitone)]);
        CHECK(spell(semitone, ChordSpelling::Mixed) == mixedNames[static_cast<size_t>(semitone)]);
    }
}

TEST_CASE("semitones wrap around the octave", "[notation][chord]")
{
    CHECK(spell(-1, ChordSpelling::Sharp) == "B");
    CHECK(spell(13, ChordSpelling::Flat) == "Db");
    CHECK(spell(-13, ChordSpelling::Mixed) == "B");
}

TEST_CASE("the key signature decides the spelling", "[notation][chord]")
{
    CHECK(ChordSymbol::spellingForKeySignature(3) == ChordSpelling::Sharp);
    CHECK(ChordSymbol::spellingForKeySignature(-2) == ChordSpelling::Flat);
    CHECK(ChordSymbol::spellingForKeySignature(0) == ChordSpelling::Mixed);
}

TEST_CASE("invalid values are replaced by defaults", "[notation][chord]")
{
    CHECK(ChordSymbol::normalizeRoot(none) == 0x31);
    CHECK(ChordSymbol::normalizeRoot(0x30) == 0x31);
    CHECK(ChordSymbol::normalizeRoot(0x23) == 0x23);
    CHECK(ChordSymbol::normalizeType(-1) == 0);
    CHECK(ChordSymbol::normalizeType(ChordChange::typeCount) == 0);
    CHECK(ChordSymbol::normalizeType(33) == 33);
    CHECK(ChordSymbol::normalizeBassRoot(0x30) == none);
    CHECK(ChordSymbol::normalizeBassRoot(none) == none);
    CHECK(ChordSymbol::normalizeBassRoot(0x32) == 0x32);
}

TEST_CASE("chords are written as root, type and bass", "[notation][chord]")
{
    CHECK(ChordSymbol::toString({0, 0x31, 0, none, none}) == "C");
    CHECK(ChordSymbol::toString({0, 0x23, 10, none, none}) == "Ebm7");
    CHECK(ChordSymbol::toString({0, 0x31, 0, 0x35, 0}) == "C/G");
    CHECK(ChordSymbol::toString({0, 0x31, 2, 0x30, 0}) == "CM7");
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
