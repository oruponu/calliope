#pragma once

#include "model/controller/ProgramBank.h"

// A program change with the bank the instrument holds when it receives it.
struct ProgramVoice
{
    ProgramBank bank;
    int program = 0;
    bool operator==(const ProgramVoice&) const = default;
};
