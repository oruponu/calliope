#pragma once
// IWYU pragma: always_keep

#include "model/TempoChange.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <string>

namespace Catch
{
template <> struct StringMaker<TempoChange>
{
    static std::string convert(const TempoChange& tc) { return std::format("{}:{}", tc.tick, tc.bpm); }
};
} // namespace Catch
