#pragma once

struct MidiEvent
{
    enum class Type
    {
        ControlChange,
        ProgramChange,
        PitchBend,
        ChannelPressure,
        KeyPressure
    };

    Type type = Type::ControlChange;
    int tick = 0;
    int data1 = 0;
    int data2 = 0;
    // Control change and pitch bend only: the value moves in a straight line from the previous event of the same kind.
    bool ramp = false;

    bool operator==(const MidiEvent&) const = default;
};
