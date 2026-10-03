#pragma once
// IWYU pragma: always_keep

#include "model/TimeSignatureChange.h"
#include <catch2/catch_tostring.hpp>
#include <string>

namespace Catch
{
template <> struct StringMaker<TimeSignatureChange>
{
    static std::string convert(const TimeSignatureChange& ts)
    {
        return std::to_string(ts.tick) + ":" + std::to_string(ts.numerator) + "/" + std::to_string(ts.denominator);
    }
};
} // namespace Catch
