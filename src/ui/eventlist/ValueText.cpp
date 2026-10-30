#include "ui/eventlist/ValueText.h"
#include <algorithm>
#include <cctype>

namespace
{
constexpr int kMaxMagnitude = 1000000;
} // namespace

std::optional<int> ValueText::parse(std::string_view text, int minimum, int maximum)
{
    const auto isSpace = [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; };
    while (!text.empty() && isSpace(text.front()))
        text.remove_prefix(1);
    while (!text.empty() && isSpace(text.back()))
        text.remove_suffix(1);

    const bool negative = !text.empty() && text.front() == '-';
    if (negative)
        text.remove_prefix(1);
    if (text.empty())
        return std::nullopt;

    int magnitude = 0;
    for (const char c : text)
    {
        if (c < '0' || c > '9')
            return std::nullopt;
        // Stops counting far past any range, so long runs of digits cannot overflow.
        magnitude = std::min(magnitude * 10 + (c - '0'), kMaxMagnitude);
    }
    return std::clamp(negative ? -magnitude : magnitude, minimum, maximum);
}
