#pragma once

#include <cstdint>
#include <vector>

struct SysExEvent
{
    int tick = 0;
    // The bytes between F0 and F7.
    std::vector<std::uint8_t> data;

    bool operator==(const SysExEvent&) const = default;
};
