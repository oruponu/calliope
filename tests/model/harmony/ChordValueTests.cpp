#include "model/harmony/ChordValue.h"
#include "model/notation/ChordSymbol.h"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <string>

namespace
{
constexpr int none = ChordChange::none;

std::string spell(int semitone, ChordSpelling spelling)
{
    return ChordSymbol::rootToString(ChordValue::semitoneToRoot(semitone, spelling));
}

const std::array<std::string, 12> sharpNames{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
const std::array<std::string, 12> flatNames{"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
const std::array<std::string, 12> mixedNames{"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
} // namespace

TEST_CASE("only roots with a note and an accidental in range are valid", "[notation][chord]")
{
    CHECK(ChordValue::isValidRoot(0x31));
    CHECK(ChordValue::isValidRoot(0x07));
    CHECK(ChordValue::isValidRoot(0x67));
    CHECK_FALSE(ChordValue::isValidRoot(ChordChange::none));
    CHECK_FALSE(ChordValue::isValidRoot(0x30));
    CHECK_FALSE(ChordValue::isValidRoot(0x38));
    CHECK_FALSE(ChordValue::isValidRoot(0x71));
}

TEST_CASE("roots map to semitones", "[notation][chord]")
{
    CHECK(ChordValue::rootToSemitone(0x31) == 0);
    CHECK(ChordValue::rootToSemitone(0x41) == 1);
    CHECK(ChordValue::rootToSemitone(0x21) == 11);
    CHECK(ChordValue::rootToSemitone(0x37) == 11);
    CHECK(ChordValue::rootToSemitone(0x01) == 9);
    CHECK(ChordValue::rootToSemitone(0x07) == 8);
    CHECK(ChordValue::rootToSemitone(0x67) == 2);
}

TEST_CASE("invalid roots have no semitone", "[notation][chord]")
{
    CHECK(ChordValue::rootToSemitone(none) == -1);
    CHECK(ChordValue::rootToSemitone(0x30) == -1);
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
    CHECK(ChordValue::spellingForKeySignature(3) == ChordSpelling::Sharp);
    CHECK(ChordValue::spellingForKeySignature(-2) == ChordSpelling::Flat);
    CHECK(ChordValue::spellingForKeySignature(0) == ChordSpelling::Mixed);
}

TEST_CASE("invalid values are replaced by defaults", "[notation][chord]")
{
    CHECK(ChordValue::normalizeRoot(none) == 0x31);
    CHECK(ChordValue::normalizeRoot(0x30) == 0x31);
    CHECK(ChordValue::normalizeRoot(0x23) == 0x23);
    CHECK(ChordValue::normalizeType(-1) == 0);
    CHECK(ChordValue::normalizeType(ChordChange::typeCount) == 0);
    CHECK(ChordValue::normalizeType(33) == 33);
    CHECK(ChordValue::normalizeBassRoot(0x30) == none);
    CHECK(ChordValue::normalizeBassRoot(none) == none);
    CHECK(ChordValue::normalizeBassRoot(0x32) == 0x32);
}
