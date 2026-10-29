#pragma once
// IWYU pragma: always_keep

#include "model/grid/GridLine.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <string>

namespace Catch
{
template <> struct StringMaker<GridLine>
{
    static std::string convert(const GridLine& line)
    {
        constexpr const char* kinds[] = {"Bar", "Beat", "Sub"};
        return std::format("{} {}", line.tick, kinds[static_cast<int>(line.kind)]);
    }
};
} // namespace Catch
