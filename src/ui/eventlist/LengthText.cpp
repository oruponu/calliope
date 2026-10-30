#include "ui/eventlist/LengthText.h"
#include <algorithm>
#include <cctype>

std::optional<int> LengthText::parse(std::string_view text)
{
    const auto isSpace = [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; };
    while (!text.empty() && isSpace(text.front()))
        text.remove_prefix(1);
    while (!text.empty() && isSpace(text.back()))
        text.remove_suffix(1);
    if (text.empty())
        return std::nullopt;

    int value = 0;
    for (const char c : text)
    {
        if (c < '0' || c > '9')
            return std::nullopt;
        // Stops counting past the largest, so long runs of digits cannot overflow.
        value = std::min(value * 10 + (c - '0'), kMaxLength);
    }
    return std::max(value, 1);
}
