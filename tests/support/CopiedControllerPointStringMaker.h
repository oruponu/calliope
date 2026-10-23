#pragma once
// IWYU pragma: always_keep

#include "edit/CopiedControllerPoints.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <string>

namespace Catch
{
template <> struct StringMaker<CopiedControllerPoint>
{
    static std::string convert(const CopiedControllerPoint& p)
    {
        return std::format("{}:{}{}", p.tick, p.value, p.ramp ? " ramp" : "");
    }
};

template <> struct StringMaker<CopiedControllerPoints>
{
    static std::string convert(const CopiedControllerPoints& c)
    {
        const auto kind =
            c.kind.type == MidiEvent::Type::PitchBend ? std::string("PB") : std::format("CC{}", c.kind.number);
        return kind + " " + StringMaker<std::vector<CopiedControllerPoint>>::convert(c.points);
    }
};
} // namespace Catch
