#pragma once

#include "model/track/TrackId.h"

struct HeldTakeNote
{
    TrackId track{};
    int noteNumber = 0;
    int velocity = 0;
    int startTick = 0;
    int id = 0;

    bool operator==(const HeldTakeNote&) const = default;
};
