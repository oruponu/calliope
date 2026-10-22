#pragma once

#include "model/MidiEvent.h"
#include <algorithm>
#include <optional>

struct ControllerKind
{
    MidiEvent::Type type = MidiEvent::Type::ControlChange;
    // Controller number; 0 for pitch bend.
    int number = 0;

    static ControllerKind controlChange(int number) { return {MidiEvent::Type::ControlChange, number}; }
    static ControllerKind pitchBend() { return {MidiEvent::Type::PitchBend, 0}; }

    static std::optional<ControllerKind> of(const MidiEvent& event)
    {
        if (event.type == MidiEvent::Type::ControlChange)
            return controlChange(event.data1);
        if (event.type == MidiEvent::Type::PitchBend)
            return pitchBend();
        return std::nullopt;
    }

    bool matches(const MidiEvent& event) const
    {
        return event.type == type && (type != MidiEvent::Type::ControlChange || event.data1 == number);
    }

    int valueOf(const MidiEvent& event) const { return type == MidiEvent::Type::PitchBend ? event.data1 : event.data2; }

    int maxValue() const { return type == MidiEvent::Type::PitchBend ? 16383 : 127; }

    // Filling in values between points would send bank changes, parameter edits or channel mode messages.
    bool canRamp() const
    {
        if (type == MidiEvent::Type::PitchBend)
            return true;
        const bool bankSelect = number == 0 || number == 32;
        const bool parameter = number == 6 || number == 38 || (number >= 96 && number <= 101);
        const bool channelMode = number >= 120;
        return !bankSelect && !parameter && !channelMode;
    }

    MidiEvent point(int tick, int value) const
    {
        MidiEvent event{.type = type, .tick = tick, .ramp = canRamp()};
        const int clamped = std::clamp(value, 0, maxValue());
        if (type == MidiEvent::Type::PitchBend)
            event.data1 = clamped;
        else
        {
            event.data1 = number;
            event.data2 = clamped;
        }
        return event;
    }

    bool operator==(const ControllerKind&) const = default;
};
