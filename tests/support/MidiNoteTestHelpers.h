#pragma once
// IWYU pragma: always_keep

#include "model/MidiNote.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <string>

namespace Catch
{
template <> struct StringMaker<MidiNote>
{
    static std::string convert(const MidiNote& n)
    {
        return std::format("{}:note={} vel={} len={}", n.startTick, n.noteNumber, n.velocity, n.duration);
    }
};
} // namespace Catch
