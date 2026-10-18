#pragma once

#include "ui/widgets/SegmentedControl.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class MetronomeSettingsComponent : public juce::Component
{
public:
    MetronomeSettingsComponent();

    // These only update what is shown; the callbacks below fire on user changes alone.
    void setMetronomeEnabled(bool enabled);
    void setVolume(int percent);
    void setCountInEnabled(bool enabled);
    void setCountInBars(int bars);

    std::function<void(bool enabled)> onMetronomeEnabledChanged;
    std::function<void(int percent)> onVolumeChanged;
    std::function<void(bool enabled)> onCountInEnabledChanged;
    std::function<void(int bars)> onCountInBarsChanged;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void showVolumeValue();
    void updateDimming();

    juce::ToggleButton metronomeToggle{"Metronome"};
    juce::Slider volumeSlider;
    juce::Label volumeValueLabel;
    juce::ToggleButton countInToggle{"Count-In"};
    SegmentedControl barsControl{{"1", "2", "3", "4"}};
    juce::Label barsLabel{"", "bars"};

    static constexpr int width = 320;
    static constexpr int height = 92;
    static constexpr int padding = 16;
    static constexpr int rowHeight = 24;
    static constexpr int rowGap = 12;
    static constexpr int gap = 8;
    static constexpr int volumeValueWidth = 40;
    static constexpr int toggleWidth = 104;
    static constexpr int barsControlWidth = 112;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetronomeSettingsComponent)
};
