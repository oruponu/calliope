#include "model/notation/NoteName.h"
#include <algorithm>
#include <cctype>

namespace
{
constexpr int kMaxNote = 127;

std::string_view trimmed(std::string_view text)
{
    const auto isSpace = [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; };
    while (!text.empty() && isSpace(text.front()))
        text.remove_prefix(1);
    while (!text.empty() && isSpace(text.back()))
        text.remove_suffix(1);
    return text;
}

// Stops counting past the MIDI range, so long runs of digits cannot overflow.
std::optional<int> digitsValue(std::string_view text)
{
    if (text.empty())
        return std::nullopt;
    int value = 0;
    for (const char c : text)
    {
        if (c < '0' || c > '9')
            return std::nullopt;
        value = std::min(value * 10 + (c - '0'), kMaxNote * 10);
    }
    return value;
}

std::optional<int> pitchClassOf(char letter)
{
    switch (std::toupper(static_cast<unsigned char>(letter)))
    {
    case 'C':
        return 0;
    case 'D':
        return 2;
    case 'E':
        return 4;
    case 'F':
        return 5;
    case 'G':
        return 7;
    case 'A':
        return 9;
    case 'B':
        return 11;
    }
    return std::nullopt;
}
} // namespace

namespace NoteName
{
std::string format(int noteNumber)
{
    static const char* names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return std::string(names[noteNumber % 12]) + std::to_string(noteNumber / 12 - 1);
}

std::optional<int> parse(std::string_view text)
{
    text = trimmed(text);
    if (const auto number = digitsValue(text))
        return std::min(*number, kMaxNote);

    if (text.empty())
        return std::nullopt;
    const auto pitchClass = pitchClassOf(text.front());
    if (!pitchClass)
        return std::nullopt;
    text.remove_prefix(1);

    int accidental = 0;
    if (!text.empty() && (text.front() == '#' || text.front() == 'b' || text.front() == 'B'))
    {
        accidental = text.front() == '#' ? 1 : -1;
        text.remove_prefix(1);
    }

    const bool belowZero = !text.empty() && text.front() == '-';
    if (belowZero)
        text.remove_prefix(1);
    const auto octave = digitsValue(text);
    if (!octave)
        return std::nullopt;

    const int note = ((belowZero ? -*octave : *octave) + 1) * 12 + *pitchClass + accidental;
    return std::clamp(note, 0, kMaxNote);
}
} // namespace NoteName
