#pragma once
// IWYU pragma: always_keep

#include "model/ChordChange.h"
#include "notation/ChordSymbol.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <string>

namespace Catch
{
template <> struct StringMaker<ChordChange>
{
    static std::string convert(const ChordChange& c)
    {
        auto text = std::to_string(c.tick) + ":";
        if (c == ChordChange::noChord(c.tick))
            return text + "N.C.";

        const auto name = ChordSymbol::toString(c);
        const bool plain = c.bassRoot == ChordChange::none && c.bassType == ChordChange::none;
        if (!name.empty() && plain)
            return text + name;

        return text + (name.empty() ? "N.C." : name) +
               std::format(" raw{{{:#04x},{},{:#04x},{:#04x}}}", c.chordRoot, c.chordType, c.bassRoot, c.bassType);
    }
};
} // namespace Catch
