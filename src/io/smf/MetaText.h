#pragma once

#include <cstdint>
#include <juce_core/juce_core.h>

namespace MetaText
{
// UTF-8 when the bytes are valid UTF-8, otherwise the system's default code page.
juce::String decode(const std::uint8_t* data, int length);
} // namespace MetaText
