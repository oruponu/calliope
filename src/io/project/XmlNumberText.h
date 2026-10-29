#pragma once

#include <optional>
#include <string>
#include <string_view>

// Numbers in project file attributes: the whole text must be the number, and doubles come back exactly.
namespace XmlNumberText
{
std::optional<int> parseInt(std::string_view text);
std::optional<double> parseFiniteDouble(std::string_view text);
std::string formatDouble(double value);
} // namespace XmlNumberText
