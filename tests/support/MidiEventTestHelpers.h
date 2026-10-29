#pragma once
// IWYU pragma: always_keep

#include "model/event/MidiEvent.h"
#include <catch2/catch_tostring.hpp>
#include <format>
#include <string>

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
        return std::format("{}:{} {} {}{}", e.tick, type, e.data1, e.data2, e.ramp ? " ramp" : "");
    }
};
} // namespace Catch
