#pragma once

#include <optional>

// A 14-bit value sent as two controllers, either of which may not have been sent yet.
struct ChaseMsbLsb
{
    std::optional<int> msb;
    std::optional<int> lsb;
};
