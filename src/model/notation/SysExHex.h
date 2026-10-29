#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace SysExHex
{
// Two uppercase hex digits per byte, separated by single spaces.
std::string format(const std::vector<std::uint8_t>& bytes);

// The inverse of format; nullopt for empty text or anything format would not produce.
std::optional<std::vector<std::uint8_t>> parse(std::string_view text);
} // namespace SysExHex
