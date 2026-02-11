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
    int channel = 1; // 1-16
    int tick = 0;
    int data1 = 0;
    int data2 = 0;
};
