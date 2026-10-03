#pragma once

#include "edit/ChordTrackEdits.h"
#include "notation/ChordSymbol.h"
#include "support/ChordStringMaker.h"
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

namespace chordtest
{
constexpr int grid = 240;

inline ChordChange chord(int tick, const std::string& name)
{
    if (name == "N.C.")
        return {tick, ChordChange::none, ChordChange::typeCount, ChordChange::none, ChordChange::none};

    size_t split = 1;
    while (split < name.size() && (name[split] == '#' || name[split] == 'b'))
        ++split;

    int root = 0;
    int type = 0;
    if (name.empty() || !ChordSymbol::rootFromString(name.substr(0, split), root) ||
        !ChordSymbol::typeFromString(name.substr(split), type))
        throw std::invalid_argument("unknown chord name: " + name);

    return {tick, root, type, ChordChange::none, ChordChange::none};
}

inline ChordChange xfNoChord(int tick)
{
    return {tick, ChordChange::none, 0x7F, ChordChange::none, ChordChange::none};
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
