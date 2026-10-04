#include "io/XmlNumberText.h"
#include <array>
#include <charconv>
#include <cmath>
#include <system_error>

namespace
{
template <typename T> std::optional<T> parseWhole(std::string_view text)
{
    T value{};
    const char* last = text.data() + text.size();
    const auto [end, error] = std::from_chars(text.data(), last, value);
    if (error != std::errc{} || end != last)
        return std::nullopt;
    return value;
}
} // namespace

namespace XmlNumberText
{
std::optional<int> parseInt(std::string_view text)
{
    return parseWhole<int>(text);
}

std::optional<double> parseFiniteDouble(std::string_view text)
{
    const auto value = parseWhole<double>(text);
    if (!value || !std::isfinite(*value))
        return std::nullopt;
    return value;
}

std::string formatDouble(double value)
{
    std::array<char, 32> buffer{};
    const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    return error == std::errc{} ? std::string(buffer.data(), end) : std::string{};
}
} // namespace XmlNumberText
