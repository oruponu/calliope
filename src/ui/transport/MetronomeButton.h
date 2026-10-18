#pragma once

#include "ui/transport/TransportButton.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class MetronomeButton : public TransportButton
{
public:
    MetronomeButton() : TransportButton(TransportButton::Metronome) {}

    std::function<void()> onMenuRequested;

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && onMenuRequested)
            onMenuRequested();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        // The menu click must not also toggle the metronome.
        if (!e.mods.isPopupMenu())
            TransportButton::mouseUp(e);
    }
};
