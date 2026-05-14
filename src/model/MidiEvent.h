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
};
