#pragma once
// IWYU pragma: always_keep

#include "model/MetaEvent.h"
#include "model/SysExHex.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <string>

namespace Catch
{
template <> struct StringMaker<MetaEvent>
{
    static std::string convert(const MetaEvent& event)
    {
        return std::format("{}:{:02X}:{}", event.tick, event.type, SysExHex::format(event.data));
    }
};
} // namespace Catch
