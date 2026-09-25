#pragma once

struct TempoChange
{
    int tick;
    double bpm;
    bool operator==(const TempoChange&) const = default;
};
