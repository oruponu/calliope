#pragma once

#include <cstdint>
#include <juce_core/juce_core.h>
#include <vector>

namespace MetaEventDescription
{
// The event list's text for a meta event: the type's name and the decoded text, or the bytes in hex.
juce::String describe(int type, const std::vector<std::uint8_t>& data);
} // namespace MetaEventDescription
