#pragma once

#include "audio/MetronomeVolume.h"
#include "engine/metronome/CountInBars.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class Metronome;
class MetronomeSettingsComponent;
class PlaybackEngine;
class ToolWindow;
class TransportBarComponent;

// Keeps the metronome volume and count-in settings, and the window that edits them together with the metronome
// switch.
class MetronomeSettingsController
{
public:
    // Reads the saved settings into transportBar and metronome, and sets transportBar's onMetronomeToggled and
    // onMetronomeMenuRequested.
    MetronomeSettingsController(TransportBarComponent& transportBar, const PlaybackEngine& playbackEngine,
                                Metronome& metronome, juce::ApplicationCommandManager& commandManager,
                                juce::Component& windowOwner, juce::ApplicationCommandTarget& commandTarget);
    ~MetronomeSettingsController();

    void toggleCountIn();
    bool isCountInEnabled() const;
    void showWindow();
    // Must be called before the owner's window goes, because Windows destroys owned windows along with their owner.
    void closeWindow();

private:
    void setMetronomeVolume(int percent);
    void setCountInEnabled(bool enabled);
    void setCountInBars(int bars);
    void updateMetronomeSettings();

    TransportBarComponent& transportBar;
    const PlaybackEngine& playbackEngine;
    Metronome& metronome;
    juce::ApplicationCommandManager& commandManager;
    juce::Component& windowOwner;
    juce::ApplicationCommandTarget& commandTarget;
    int metronomeVolume = MetronomeVolume::defaultPercent;
    bool countInEnabled = false;
    int countInBars = CountInBars::defaultBars;
    std::unique_ptr<ToolWindow> metronomeSettingsWindow;
    // Owned by metronomeSettingsWindow.
    MetronomeSettingsComponent* metronomeSettings = nullptr;

    JUCE_DECLARE_NON_COPYABLE(MetronomeSettingsController)
};
