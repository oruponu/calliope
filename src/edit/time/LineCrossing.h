#pragma once

#include "model/event/MidiEvent.h"

struct LineCrossing
{
    // At the tick, with the line's value there, made by ControllerKind::point.
    MidiEvent point;
    // The line's next point sits exactly at the tick.
    bool nextAtTick = false;

    bool operator==(const LineCrossing&) const = default;
};
