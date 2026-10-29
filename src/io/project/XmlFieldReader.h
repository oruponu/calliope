#pragma once

#include <array>
#include <cstddef>
#include <juce_core/juce_core.h>
#include <string>
#include <utility>

// Reads the attributes of one project file. A missing or malformed value does not stop the read; it marks the
// whole read as failed, and the caller discards the result.
class XmlFieldReader
{
public:
    int integer(const juce::XmlElement& element, const char* name);
    double finiteDouble(const juce::XmlElement& element, const char* name);
    bool flag(const juce::XmlElement& element, const char* name);
    std::string text(const juce::XmlElement& element, const char* name);

    template <typename Enum, std::size_t N>
    Enum choice(const juce::XmlElement& element, const char* name,
                const std::array<std::pair<Enum, const char*>, N>& labels)
    {
        const auto value = element.getStringAttribute(name);
        for (const auto& [candidate, label] : labels)
            if (value == label)
                return candidate;
        failed = true;
        return labels.front().first;
    }

    void require(bool condition);
    bool ok() const { return !failed; }

private:
    bool failed = false;
};
