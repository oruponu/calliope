#pragma once

#include "model/MidiSequence.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

namespace chordtest
{
constexpr int grid = 240;

inline bool isCanonicalNoChord(const ChordChange& c)
{
    return c.chordRoot == MidiSequence::chordNone && c.chordType == MidiSequence::chordTypeCount &&
           c.bassRoot == MidiSequence::chordNone && c.bassType == MidiSequence::chordNone;
}

inline ChordChange chord(int tick, const std::string& name)
{
    if (name == "N.C.")
        return {tick, MidiSequence::chordNone, MidiSequence::chordTypeCount, MidiSequence::chordNone,
                MidiSequence::chordNone};

    size_t split = 1;
    while (split < name.size() && (name[split] == '#' || name[split] == 'b'))
        ++split;

    int root = 0;
    int type = 0;
    if (name.empty() || !MidiSequence::chordRootFromString(name.substr(0, split), root) ||
        !MidiSequence::chordTypeFromString(name.substr(split), type))
        throw std::invalid_argument("unknown chord name: " + name);

    return {tick, root, type, MidiSequence::chordNone, MidiSequence::chordNone};
}

inline ChordChange xfNoChord(int tick)
{
    return {tick, MidiSequence::chordNone, 0x7F, MidiSequence::chordNone, MidiSequence::chordNone};
}

struct Entry
{
    int tick;
    std::string name;
};

inline std::vector<ChordChange> chords(std::initializer_list<Entry> entries)
{
    std::vector<ChordChange> result;
    for (const auto& e : entries)
        result.push_back(chord(e.tick, e.name));
    return result;
}

inline RelativeChord relative(int tickOffset, int length, const std::string& name)
{
    const auto c = chord(0, name);
    return {tickOffset, length, c.chordRoot, c.chordType, c.bassRoot, c.bassType};
}
} // namespace chordtest

namespace Catch
{
template <> struct StringMaker<ChordChange>
{
    static std::string convert(const ChordChange& c)
    {
        auto text = std::to_string(c.tick) + ":";
        if (chordtest::isCanonicalNoChord(c))
            return text + "N.C.";

        const auto name = MidiSequence::chordToString(c);
        const bool plain = c.bassRoot == MidiSequence::chordNone && c.bassType == MidiSequence::chordNone;
        if (!name.empty() && plain)
            return text + name;

        return text + (name.empty() ? "N.C." : name) +
               std::format(" raw{{{:#04x},{},{:#04x},{:#04x}}}", c.chordRoot, c.chordType, c.bassRoot, c.bassType);
    }
};
} // namespace Catch
