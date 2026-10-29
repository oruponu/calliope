#include "model/notation/ChordSymbol.h"
#include "model/harmony/ChordValue.h"
#include <cctype>
#include <ranges>

namespace
{
const char* const chordNoteNames[] = {"", "C", "D", "E", "F", "G", "A", "B"};
const char* const chordAccidentals[] = {"bbb", "bb", "b", "", "#", "##", "###"};
const char* const chordTypeNames[] = {"",     "6",       "maj7",  "maj7#11", "add9", "maj9", "6add9", "aug",   "m",
                                      "m6",   "m7",      "m7b5",  "madd9",   "m9",   "m11",  "mmaj7", "mmaj9", "dim",
                                      "dim7", "7",       "7sus4", "7b5",     "9",    "7#11", "13",    "7b9",   "7b13",
                                      "7#9",  "augmaj7", "aug7",  "1+8",     "5",    "sus4", "sus2",  ""};
} // namespace

std::string ChordSymbol::toString(const ChordChange& chord)
{
    auto parts = toParts(chord);
    std::string result = parts.root + parts.type;
    if (!parts.bass.empty())
    {
        result += "/";
        result += parts.bass;
    }

    return result;
}

ChordSymbolParts ChordSymbol::toParts(const ChordChange& chord)
{
    if (chord.isNoChord())
        return {};

    std::string root = rootToString(chord.chordRoot);
    if (root.empty())
        return {"--", {}, {}};

    return {root, typeToString(chord.chordType), rootToString(chord.bassRoot)};
}

std::string ChordSymbol::rootToString(int root)
{
    if (!ChordValue::isValidRoot(root))
        return {};

    int noteIndex = root & 0x0F;
    int accIndex = (root >> 4) & 0x07;
    std::string result = chordNoteNames[noteIndex];
    result += chordAccidentals[accIndex];
    return result;
}

bool ChordSymbol::rootFromString(const std::string& text, int& root)
{
    auto s = text | std::views::filter([](char c) { return !std::isspace(static_cast<unsigned char>(c)); }) |
             std::ranges::to<std::string>();

    if (s.empty())
        return false;

    s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    for (size_t i = 1; i < s.size(); ++i)
        s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));

    int noteIndex = 0;
    for (int i = 1; i <= 7; ++i)
        if (chordNoteNames[i][0] == s[0])
            noteIndex = i;
    if (noteIndex == 0)
        return false;

    int sharps = 0;
    int flats = 0;
    for (size_t i = 1; i < s.size(); ++i)
    {
        if (s[i] == '#')
            ++sharps;
        else if (s[i] == 'b')
            ++flats;
        else
            return false;
    }
    if ((sharps > 0 && flats > 0) || sharps > 3 || flats > 3)
        return false;

    root = ((3 + sharps - flats) << 4) | noteIndex;
    return true;
}

std::string ChordSymbol::typeToString(int type)
{
    if (type < 0 || type >= ChordChange::typeCount)
        return {};

    return chordTypeNames[type];
}

bool ChordSymbol::typeFromString(const std::string& text, int& type)
{
    auto s = text | std::views::filter([](char c) { return !std::isspace(static_cast<unsigned char>(c)); }) |
             std::ranges::to<std::string>();

    for (int i = 0; i < ChordChange::typeCount; ++i)
    {
        if (s == chordTypeNames[i])
        {
            type = i;
            return true;
        }
    }
    return false;
}
