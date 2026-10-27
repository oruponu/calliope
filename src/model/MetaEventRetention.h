#pragma once

#include <cstdint>
#include <vector>

namespace MetaEventRetention
{
enum class Owner
{
    Song,
    Track
};

// Whether a meta event is kept as bytes, rather than read into the model or written by the app itself.
bool isRetained(int type, const std::vector<std::uint8_t>& data, Owner owner);
} // namespace MetaEventRetention
