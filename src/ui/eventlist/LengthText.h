#pragma once

#include <optional>
#include <string_view>

namespace LengthText
{
constexpr int kMaxLength = 9999999;

// A number of ticks, kept inside 1 to kMaxLength; nullopt for anything else.
std::optional<int> parse(std::string_view text);
} // namespace LengthText
