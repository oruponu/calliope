#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>
#include <string>
#include <string_view>

struct ValueFormat
{
    std::function<std::string(int value)> format;
    std::function<std::optional<int>(std::string_view text)> parse;
    int minimum = 0;
    int maximum = 0;
    int maxLength = 0;
    juce::String allowedCharacters;
    juce::Justification justification = juce::Justification::centredLeft;
};
