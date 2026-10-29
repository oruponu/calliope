#pragma once
// IWYU pragma: always_keep

#include "edit/controller/CopiedControllerPoints.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <string>

namespace Catch
{
template <> struct StringMaker<CopiedControllerPoint>
{
    static std::string convert(const CopiedControllerPoint& p)
    {
        auto text = std::format("{}:{}{}", p.tick, p.value, p.ramp ? " ramp" : "");
        if (p.bankMsb.has_value() || p.bankLsb.has_value())
        {
            const auto msb = p.bankMsb.has_value() ? std::to_string(*p.bankMsb) : "-";
            const auto lsb = p.bankLsb.has_value() ? std::to_string(*p.bankLsb) : "-";
            text += std::format(" bank {}:{}", msb, lsb);
        }
        return text;
    }
};

template <> struct StringMaker<CopiedControllerPoints>
{
    static std::string convert(const CopiedControllerPoints& c)
    {
        const auto kind = c.kind.type == MidiEvent::Type::PitchBend       ? std::string("PB")
                          : c.kind.type == MidiEvent::Type::ProgramChange ? std::string("PC")
                                                                          : std::format("CC{}", c.kind.number);
        return kind + " " + StringMaker<std::vector<CopiedControllerPoint>>::convert(c.points);
    }
};
} // namespace Catch
