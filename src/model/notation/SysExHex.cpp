#include "model/notation/SysExHex.h"

namespace
{
int hexValue(char character)
{
    if (character >= '0' && character <= '9')
        return character - '0';
    if (character >= 'A' && character <= 'F')
        return character - 'A' + 10;
    return -1;
}
} // namespace

namespace SysExHex
{
std::string format(const std::vector<std::uint8_t>& bytes)
{
    constexpr char digits[] = "0123456789ABCDEF";

    std::string text;
    if (!bytes.empty())
        text.reserve(bytes.size() * 3 - 1);

    for (const auto byte : bytes)
    {
        if (!text.empty())
            text.push_back(' ');
        text.push_back(digits[byte >> 4]);
        text.push_back(digits[byte & 0x0F]);
    }
    return text;
}

std::optional<std::vector<std::uint8_t>> parse(std::string_view text)
{
    if (text.empty() || text.size() % 3 != 2)
        return std::nullopt;

    std::vector<std::uint8_t> bytes;
    bytes.reserve((text.size() + 1) / 3);
    for (std::size_t index = 0; index < text.size(); index += 3)
    {
        if (index > 0 && text[index - 1] != ' ')
            return std::nullopt;

        const int high = hexValue(text[index]);
        const int low = hexValue(text[index + 1]);
        if (high < 0 || low < 0)
            return std::nullopt;
        bytes.push_back(static_cast<std::uint8_t>((high << 4) | low));
    }
    return bytes;
}
} // namespace SysExHex
