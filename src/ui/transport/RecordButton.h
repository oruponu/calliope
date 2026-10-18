#pragma once

#include "ui/transport/TransportButton.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <utility>

class RecordButton : public TransportButton, private juce::Timer
{
public:
    RecordButton() : TransportButton(TransportButton::Record) {}

    void setRecording(bool isRecording)
    {
        recording = isRecording;
        refresh();
    }

    // Blinks while recording, until `condition` stops holding.
    void blinkWhile(std::function<bool()> condition)
    {
        blinkCondition = std::move(condition);
        blinkStartMs = juce::Time::getMillisecondCounter();
        startTimerHz(pollRateHz);
        refresh();
    }

private:
    // Polled faster than it blinks, so the button turns steady soon after the condition ends.
    static constexpr int pollRateHz = 30;
    static constexpr juce::uint32 blinkHalfPeriodMs = 250;

    void timerCallback() override
    {
        if (!blinkCondition())
        {
            stopTimer();
            blinkCondition = nullptr;
        }
        refresh();
    }

    void refresh()
    {
        const bool lit =
            !isTimerRunning() || (juce::Time::getMillisecondCounter() - blinkStartMs) / blinkHalfPeriodMs % 2 == 0;
        if (isActive() != (recording && lit))
            setActive(recording && lit);
    }

    std::function<bool()> blinkCondition;
    juce::uint32 blinkStartMs = 0;
    bool recording = false;
};
