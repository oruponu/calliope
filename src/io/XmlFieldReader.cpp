#include "io/XmlFieldReader.h"
#include "io/XmlNumberText.h"
#include <optional>

namespace
{
std::optional<std::string> attributeText(const juce::XmlElement& element, const char* name)
{
    if (!element.hasAttribute(name))
        return std::nullopt;
    return element.getStringAttribute(name).toStdString();
}
} // namespace

int XmlFieldReader::integer(const juce::XmlElement& element, const char* name)
{
    const auto text = attributeText(element, name);
    const auto value = text ? XmlNumberText::parseInt(*text) : std::nullopt;
    require(value.has_value());
    return value.value_or(0);
}

double XmlFieldReader::finiteDouble(const juce::XmlElement& element, const char* name)
{
    const auto text = attributeText(element, name);
    const auto value = text ? XmlNumberText::parseFiniteDouble(*text) : std::nullopt;
    require(value.has_value());
    return value.value_or(0.0);
}

bool XmlFieldReader::flag(const juce::XmlElement& element, const char* name)
{
    const auto text = attributeText(element, name);
    require(text == "0" || text == "1");
    return text == "1";
}

std::string XmlFieldReader::text(const juce::XmlElement& element, const char* name)
{
    require(element.hasAttribute(name));
    return element.getStringAttribute(name).toStdString();
}

void XmlFieldReader::require(bool condition)
{
    if (!condition)
        failed = true;
}
