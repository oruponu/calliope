#pragma once

struct TimeSignatureChange
{
    int tick;
    int numerator;
    int denominator;
    bool operator==(const TimeSignatureChange&) const = default;
};
