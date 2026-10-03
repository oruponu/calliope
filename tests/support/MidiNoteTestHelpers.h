#pragma once

#include "model/MidiNote.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <string>

inline bool operator==(const MidiNote& a, const MidiNote& b)
{
    return a.noteNumber == b.noteNumber && a.velocity == b.velocity && a.startTick == b.startTick &&
           a.duration == b.duration;
}

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
