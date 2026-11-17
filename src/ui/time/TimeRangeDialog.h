#pragma once

#include "model/timeline/BarBeatTick.h"
#include "model/timeline/TimelineMap.h"
#include "ui/time/TimeRangeDialogTexts.h"
#include <array>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>
#include <utility>

class TimeRangeDialog : public juce::Component
{
public:
    // apply returns the reason the edit cannot be made, which keeps the dialog open, or nothing to close it.
    static void launch(const TimeRangeDialogTexts& texts, const TimelineMap& timeline, BarBeatTick initialStart,
                       std::function<std::optional<juce::String>(int start, int end)> apply, juce::Component& owner);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using Fields = std::array<juce::TextEditor, 3>;

    TimeRangeDialog(const TimeRangeDialogTexts& texts, const TimelineMap& initialTimeline, BarBeatTick initialStart,
                    std::function<std::optional<juce::String>(int, int)> onApply);

    void setUpFields(Fields& fields, const BarBeatTick& value);
    static void showPosition(Fields& fields, const BarBeatTick& value);
    void layoutFields(Fields& fields, juce::Rectangle<int> row);
    std::optional<BarBeatTick> readPosition(const Fields& fields) const;
    std::optional<std::pair<int, int>> readRange() const;
    void showKeptInside(Fields& fields);
    void onFieldsChanged();
    void showError(const juce::String& reason);
    void applyAndClose();
    void close();

    TimelineMap timeline;
    std::function<std::optional<juce::String>(int, int)> apply;
    juce::Label startLabel{"", "Start"};
    juce::Label endLabel{"", "End"};
    Fields startFields;
    Fields endFields;
    juce::Label errorLabel;
    juce::TextButton applyButton;
    juce::TextButton cancelButton{"Cancel"};

    static constexpr int padding = 16;
    static constexpr int rowHeight = 24;
    static constexpr int errorHeight = 32;
    static constexpr int rowGap = 12;
    static constexpr int errorGap = 8;
    static constexpr int buttonRowGap = 16;
    static constexpr int gap = 8;
    static constexpr int labelWidth = 48;
    static constexpr int barWidth = 52;
    static constexpr int beatWidth = 36;
    static constexpr int tickWidth = 64;
    static constexpr int dotWidth = 8;
    static constexpr int buttonWidth = 80;
    static constexpr int width = padding * 2 + labelWidth + gap + barWidth + beatWidth + tickWidth + dotWidth * 2;
    static constexpr int height = padding * 2 + rowHeight * 3 + rowGap + buttonRowGap;
    static constexpr int heightWithError = height + errorGap + errorHeight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeRangeDialog)
};
