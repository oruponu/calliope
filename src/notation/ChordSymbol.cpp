#include "notation/ChordSymbol.h"
#include <cctype>

namespace
{
const char* const chordNoteNames[] = {"", "C", "D", "E", "F", "G", "A", "B"};
const char* const chordAccidentals[] = {"bbb", "bb", "b", "", "#", "##", "###"};
const char* const chordTypeNames[] = {
    "",      "6",     "M7",     "M7(#11)", "add9",   "M7(9)", "6(9)", "aug", "m",     "m6",   "m7",   "m7b5",
    "madd9", "m7(9)", "m7(11)", "mM7",     "mM7(9)", "dim",   "dim7", "7",   "7sus4", "7b5",  "7(9)", "7(#11)",
    "7(13)", "7(b9)", "7(b13)", "7(#9)",   "M7aug",  "7aug",  "1+8",  "5",   "sus4",  "sus2", ""};

// semitone of each XF note index 1-7 (C, D, E, F, G, A, B)
constexpr int chordNoteSemitones[] = {-1, 0, 2, 4, 5, 7, 9, 11};

// XF root nibbles per semitone
constexpr int sharpRoots[] = {0x31, 0x41, 0x32, 0x42, 0x33, 0x34, 0x44, 0x35, 0x45, 0x36, 0x46, 0x37};
constexpr int flatRoots[] = {0x31, 0x22, 0x32, 0x23, 0x33, 0x34, 0x25, 0x35, 0x26, 0x36, 0x27, 0x37};
constexpr int mixedRoots[] = {0x31, 0x41, 0x32, 0x23, 0x33, 0x34, 0x44, 0x35, 0x26, 0x36, 0x27, 0x37};
} // namespace

std::string ChordSymbol::toString(const ChordChange& chord)
{
    if (chord.isNoChord())
        return {};

    std::string result = rootToString(chord.chordRoot);
    if (result.empty())
        return "--";

    result += typeToString(chord.chordType);

    std::string bassText = rootToString(chord.bassRoot);
    if (!bassText.empty())
    {
        result += "/";
        result += bassText;
    }

    return result;
}

std::string ChordSymbol::rootToString(int root)
{
    int noteIndex = root & 0x0F;
    int accIndex = (root >> 4) & 0x07;
    if (root == ChordChange::none || noteIndex < 1 || noteIndex > 7 || accIndex > 6)
        return {};

    std::string result = chordNoteNames[noteIndex];
    result += chordAccidentals[accIndex];
    return result;
}

bool ChordSymbol::rootFromString(const std::string& text, int& root)
{
    std::string s;
    for (char c : text)
        if (!std::isspace(static_cast<unsigned char>(c)))
            s += c;

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
    std::string s;
    for (char c : text)
        if (!std::isspace(static_cast<unsigned char>(c)))
            s += c;

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

int ChordSymbol::rootToSemitone(int root)
{
    int noteIndex = root & 0x0F;
    int accIndex = (root >> 4) & 0x07;
    if (root == ChordChange::none || noteIndex < 1 || noteIndex > 7 || accIndex > 6)
        return -1;

    int semitone = (chordNoteSemitones[noteIndex] + accIndex - 3) % 12;
    return semitone < 0 ? semitone + 12 : semitone;
}

int ChordSymbol::semitoneToRoot(int semitone, ChordSpelling spelling)
{
    semitone = ((semitone % 12) + 12) % 12;
    if (spelling == ChordSpelling::Sharp)
        return sharpRoots[semitone];
    if (spelling == ChordSpelling::Flat)
        return flatRoots[semitone];
    return mixedRoots[semitone];
}

ChordSpelling ChordSymbol::spellingForKeySignature(int sharpsOrFlats)
{
    if (sharpsOrFlats > 0)
        return ChordSpelling::Sharp;
    if (sharpsOrFlats < 0)
        return ChordSpelling::Flat;
    return ChordSpelling::Mixed;
}

int ChordSymbol::normalizeRoot(int root)
{
    return rootToString(root).empty() ? 0x31 : root;
}

int ChordSymbol::normalizeType(int type)
{
    return (type < 0 || type >= ChordChange::typeCount) ? 0 : type;
}

int ChordSymbol::normalizeBassRoot(int bassRoot)
{
    return rootToString(bassRoot).empty() ? ChordChange::none : bassRoot;
}
