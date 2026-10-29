#pragma once

#include "model/event/MidiEvent.h"
#include <vector>

// One piece of a channel's chased state, with the events that set it, in the order they are sent.
struct ChaseItem
{
    enum class Kind
    {
        Program,
        PendingBank,
        Controller,
        Parameter,
        Selection,
        PitchBend,
        ChannelPressure
    };

    Kind kind = Kind::Program;
    // The controller number for Controller; (isNrpn << 14) | (msb << 7) | lsb for Parameter; otherwise 0.
    int number = 0;
    std::vector<MidiEvent> events;
    bool operator==(const ChaseItem&) const = default;
};
