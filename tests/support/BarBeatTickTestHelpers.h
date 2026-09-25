#pragma once

#include "model/BarBeatTick.h"
#include <catch2/catch_tostring.hpp>
#include <string>

inline bool operator==(const BarBeatTick& a, const BarBeatTick& b)
{
    return a.bar == b.bar && a.beat == b.beat && a.tick == b.tick;
}

namespace Catch
{
template <> struct StringMaker<BarBeatTick>
{
    static std::string convert(const BarBeatTick& p)
    {
        return std::to_string(p.bar) + "." + std::to_string(p.beat) + "." + std::to_string(p.tick);
    }
};
} // namespace Catch
