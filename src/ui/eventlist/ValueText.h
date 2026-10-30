#pragma once

#include <optional>
#include <string_view>

namespace ValueText
{
// A whole number, kept inside minimum to maximum; nullopt for anything else.
std::optional<int> parse(std::string_view text, int minimum, int maximum);
} // namespace ValueText
