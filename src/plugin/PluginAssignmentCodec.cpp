#include "plugin/PluginAssignmentCodec.h"

namespace PluginAssignmentCodec
{
std::string toXml(const juce::PluginDescription& description)
{
    return description.createXml()->toString(juce::XmlElement::TextFormat().singleLine().withoutHeader()).toStdString();
}

std::optional<juce::PluginDescription> fromXml(const std::string& xml)
{
    auto element = juce::parseXML(juce::String::fromUTF8(xml.data(), static_cast<int>(xml.size())));
    juce::PluginDescription description;
    if (element == nullptr || !description.loadFromXml(*element))
        return std::nullopt;
    return description;
}

juce::MemoryBlock toMemoryBlock(const std::vector<std::byte>& bytes)
{
    return juce::MemoryBlock(bytes.data(), bytes.size());
}
} // namespace PluginAssignmentCodec
