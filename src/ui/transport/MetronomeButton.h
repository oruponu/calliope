#pragma once

#include "ui/theme/Theme.h"
#include "ui/transport/TransportButton.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class MetronomeButton : public TransportButton, private juce::Timer
{
public:
    MetronomeButton() : TransportButton(TransportButton::Metronome) {}

    // direction is +1 (up) or -1 (down)
    std::function<void(int direction)> onWheel;

    void showVolume(int percent)
    {
        volumeText = juce::String(percent) + "%";
        startTimer(volumeDisplayMs);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        TransportButton::paint(g);
        if (volumeText.isEmpty())
            return;

        using namespace calliope::theme;
        g.setColour(surface::surface2);
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), radius::r2);
        g.setColour(text::t1);
        g.setFont(font::mono(font::sizeSM));
        g.drawText(volumeText, getLocalBounds(), juce::Justification::centred);
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        if (onWheel && wheel.deltaY != 0.0f)
            onWheel(wheel.deltaY > 0.0f ? 1 : -1);
        else
            TransportButton::mouseWheelMove(e, wheel);
    }

private:
    static constexpr int volumeDisplayMs = 1000;

    void timerCallback() override
    {
        stopTimer();
        volumeText.clear();
        repaint();
    }

    juce::String volumeText;
};
