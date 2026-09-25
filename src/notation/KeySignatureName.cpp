#include "notation/KeySignatureName.h"
#include <cctype>

namespace
{
const char* const majorKeys[] = {"Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#"};
const char* const minorKeys[] = {"Abm", "Ebm", "Bbm", "Fm",  "Cm",  "Gm",  "Dm", "Am",
                                 "Em",  "Bm",  "F#m", "C#m", "G#m", "D#m", "A#m"};
} // namespace

std::string KeySignatureName::toString(int sharpsOrFlats, bool isMinor)
{
    int index = sharpsOrFlats + 7;
    if (index < 0 || index > 14)
        return "--";

    return isMinor ? minorKeys[index] : majorKeys[index];
}

bool KeySignatureName::fromString(const std::string& text, int& sharpsOrFlats, bool& isMinor)
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

    for (int i = 0; i < 15; ++i)
    {
        if (s == majorKeys[i])
        {
            sharpsOrFlats = i - 7;
            isMinor = false;
            return true;
        }
        if (s == minorKeys[i])
        {
            sharpsOrFlats = i - 7;
            isMinor = true;
            return true;
        }
    }

    struct Alias
    {
        const char* name;
        int sharpsOrFlats;
        bool isMinor;
    };
    static const Alias aliases[] = {
        {"D#", -3, false}, // → Eb
        {"G#", -4, false}, // → Ab
        {"A#", -2, false}, // → Bb
        {"Dbm", 4, true},  // → C#m
        {"Gbm", 3, true},  // → F#m
    };

    for (const auto& a : aliases)
    {
        if (s == a.name)
        {
            sharpsOrFlats = a.sharpsOrFlats;
            isMinor = a.isMinor;
            return true;
        }
    }

    return false;
}

int KeySignatureName::normalizeSharpsOrFlats(int sharpsOrFlats)
{
    if (sharpsOrFlats == 7)
        return -5;
    if (sharpsOrFlats == -7)
        return 5;
    return sharpsOrFlats;
}
