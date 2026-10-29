#include "ui/MetaEventDescription.h"
#include "io/MetaText.h"
#include "model/notation/SysExHex.h"
#include <format>

namespace
{
const char* textTypeName(int type)
{
    static constexpr const char* names[] = {"Text",   "Copyright", "Track Name",   "Instrument", "Lyric",
                                            "Marker", "Cue Point", "Program Name", "Device Name"};
    if (type < 0x01 || type > 0x09)
        return nullptr;
    return names[type - 1];
}
} // namespace

namespace MetaEventDescription
{
juce::String describe(int type, const std::vector<std::uint8_t>& data)
{
    if (const auto* name = textTypeName(type))
    {
        auto textBytes = data;
        for (auto& byte : textBytes)
            if (byte < 0x20)
                byte = 0x20;

        const auto text = MetaText::decode(textBytes.data(), static_cast<int>(textBytes.size()));
        return text.isEmpty() ? juce::String(name) : juce::String(name) + " " + text;
    }

    auto description = juce::String(std::format("Meta FF {:02X}", type));
    if (!data.empty())
        description += " " + juce::String(SysExHex::format(data));
    return description;
}
} // namespace MetaEventDescription
