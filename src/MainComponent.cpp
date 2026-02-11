#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize(800, 600);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setFont(24.0f);
    g.setColour(juce::Colours::white);
    g.drawText("Calliope", getLocalBounds(), juce::Justification::centred);
}

void MainComponent::resized() {}
