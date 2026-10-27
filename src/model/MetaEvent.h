#pragma once

#include <cstdint>
#include <vector>

struct MetaEvent
{
    int tick = 0;
    // The byte after FF.
    std::uint8_t type = 0;
    // As stored in the file; text is not decoded.
    std::vector<std::uint8_t> data;

    bool operator==(const MetaEvent&) const = default;
};
