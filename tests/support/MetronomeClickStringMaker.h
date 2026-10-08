#pragma once
// IWYU pragma: always_keep

#include "engine/MetronomeClicks.h"
#include <catch2/catch_tostring.hpp>
#include <string>

namespace Catch
{
template <> struct StringMaker<MetronomeClick>
{
    static std::string convert(const MetronomeClick& click)
    {
        return std::to_string(click.tick) + (click.accent ? ":accent" : ":plain");
    }
};
} // namespace Catch
