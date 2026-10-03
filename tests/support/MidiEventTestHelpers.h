#pragma once

#include "model/MidiEvent.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <string>

inline bool operator==(const MidiEvent& a, const MidiEvent& b)
{
    return a.type == b.type && a.tick == b.tick && a.data1 == b.data1 && a.data2 == b.data2;
}

namespace Catch
{
template <> struct StringMaker<MidiEvent>
{
    static std::string convert(const MidiEvent& e)
    {
        const char* type = "?";
        switch (e.type)
        {
        case MidiEvent::Type::ControlChange:
            type = "CC";
            break;
        case MidiEvent::Type::ProgramChange:
            type = "PC";
            break;
        case MidiEvent::Type::PitchBend:
            type = "PB";
            break;
        case MidiEvent::Type::ChannelPressure:
            type = "CP";
            break;
        case MidiEvent::Type::KeyPressure:
            type = "KP";
            break;
        }
        return std::format("{}:{} {} {}", e.tick, type, e.data1, e.data2);
    }
};
} // namespace Catch
