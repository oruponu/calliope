#pragma once
// IWYU pragma: always_keep

#include "model/SysExEvent.h"
#include "model/SysExHex.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <string>

namespace Catch
{
template <> struct StringMaker<SysExEvent>
{
    static std::string convert(const SysExEvent& sysEx)
    {
        return std::format("{}:{}", sysEx.tick, SysExHex::format(sysEx.data));
    }
};
} // namespace Catch
