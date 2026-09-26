#pragma once

#include <cstddef>
#include <juce_audio_processors/juce_audio_processors.h>
#include <optional>
#include <string>
#include <vector>

namespace PluginAssignmentCodec
{
std::string toXml(const juce::PluginDescription& description);
std::optional<juce::PluginDescription> fromXml(const std::string& xml);
juce::MemoryBlock toMemoryBlock(const std::vector<std::byte>& bytes);
} // namespace PluginAssignmentCodec
