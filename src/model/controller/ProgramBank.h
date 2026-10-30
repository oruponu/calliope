#pragma once

struct ProgramBank
{
    int msb = 0;
    int lsb = 0;
    bool isZero() const { return msb == 0 && lsb == 0; }
    bool operator==(const ProgramBank&) const = default;
};
