#include "ui/transport/MetronomeSettingsComponent.h"
#include "audio/MetronomeVolume.h"
#include "engine/CountInBars.h"
#include "ui/theme/Theme.h"

MetronomeSettingsComponent::MetronomeSettingsComponent()
{
    using namespace calliope::theme;

    metronomeToggle.setWantsKeyboardFocus(false);
    metronomeToggle.setMouseClickGrabsKeyboardFocus(false);
    metronomeToggle.onClick = [this]
    {
        updateDimming();
        if (onMetronomeEnabledChanged)
            onMetronomeEnabledChanged(metronomeToggle.getToggleState());
    };
    addAndMakeVisible(metronomeToggle);

    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    volumeSlider.setRange(MetronomeVolume::min, MetronomeVolume::max, 1.0);
    volumeSlider.setColour(juce::Slider::trackColourId, accent::base);
    volumeSlider.setColour(juce::Slider::thumbColourId, text::t1);
    volumeSlider.setColour(juce::Slider::backgroundColourId, surface::surface3);
    volumeSlider.setMouseClickGrabsKeyboardFocus(false);
    volumeSlider.onValueChange = [this]
    {
        showVolumeValue();
        if (onVolumeChanged)
            onVolumeChanged(static_cast<int>(volumeSlider.getValue()));
    };
    addAndMakeVisible(volumeSlider);

    volumeValueLabel.setFont(font::mono(font::sizeSM));
    volumeValueLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(volumeValueLabel);

    countInToggle.setWantsKeyboardFocus(false);
    countInToggle.setMouseClickGrabsKeyboardFocus(false);
    countInToggle.onClick = [this]
    {
        updateDimming();
        if (onCountInEnabledChanged)
            onCountInEnabledChanged(countInToggle.getToggleState());
    };
    addAndMakeVisible(countInToggle);

    barsControl.onChange = [this](int index)
    {
        if (onCountInBarsChanged)
            onCountInBarsChanged(index + CountInBars::min);
    };
    addAndMakeVisible(barsControl);

    addAndMakeVisible(barsLabel);

    setVolume(MetronomeVolume::defaultPercent);
    setCountInBars(CountInBars::defaultBars);
    updateDimming();
    setSize(width, height);
}

void MetronomeSettingsComponent::setMetronomeEnabled(bool enabled)
{
    metronomeToggle.setToggleState(enabled, juce::dontSendNotification);
    updateDimming();
}

void MetronomeSettingsComponent::setVolume(int percent)
{
    volumeSlider.setValue(percent, juce::dontSendNotification);
    showVolumeValue();
}

void MetronomeSettingsComponent::setCountInEnabled(bool enabled)
{
    countInToggle.setToggleState(enabled, juce::dontSendNotification);
    updateDimming();
}

void MetronomeSettingsComponent::setCountInBars(int bars)
{
    barsControl.setSelectedIndex(bars - CountInBars::min);
}

void MetronomeSettingsComponent::paint(juce::Graphics& g)
{
    g.fillAll(calliope::theme::surface::bg2);
}

void MetronomeSettingsComponent::resized()
{
    auto area = getLocalBounds().reduced(padding);

    auto volumeRow = area.removeFromTop(rowHeight);
    metronomeToggle.setBounds(volumeRow.removeFromLeft(toggleWidth));
    volumeRow.removeFromLeft(gap);
    volumeValueLabel.setBounds(volumeRow.removeFromRight(volumeValueWidth));
    volumeSlider.setBounds(volumeRow);

    area.removeFromTop(rowGap);
    auto countInRow = area.removeFromTop(rowHeight);
    countInToggle.setBounds(countInRow.removeFromLeft(toggleWidth));
    countInRow.removeFromLeft(gap);
    barsControl.setBounds(countInRow.removeFromLeft(barsControlWidth));
    countInRow.removeFromLeft(gap);
    barsLabel.setBounds(countInRow);
}

void MetronomeSettingsComponent::showVolumeValue()
{
    volumeValueLabel.setText(juce::String(static_cast<int>(volumeSlider.getValue())) + "%", juce::dontSendNotification);
}

void MetronomeSettingsComponent::updateDimming()
{
    using namespace calliope::theme;
    const float metronomeAlpha = metronomeToggle.getToggleState() ? 1.0f : 0.5f;
    volumeSlider.setAlpha(metronomeAlpha);
    volumeValueLabel.setAlpha(metronomeAlpha);

    const bool countInEnabled = countInToggle.getToggleState();
    barsControl.setDimmed(!countInEnabled);
    barsLabel.setColour(juce::Label::textColourId, countInEnabled ? text::t2 : text::t3);
}
