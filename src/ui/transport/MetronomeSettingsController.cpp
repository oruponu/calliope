#include "ui/transport/MetronomeSettingsController.h"
#include "audio/Metronome.h"
#include "engine/PlaybackEngine.h"
#include "settings/AppProperties.h"
#include "ui/commands/AppCommands.h"
#include "ui/transport/MetronomeSettingsComponent.h"
#include "ui/transport/TransportBarComponent.h"
#include "ui/window/ToolWindow.h"

MetronomeSettingsController::MetronomeSettingsController(TransportBarComponent& transportBarRef,
                                                         const PlaybackEngine& playbackEngineRef,
                                                         Metronome& metronomeRef,
                                                         juce::ApplicationCommandManager& commandManagerRef,
                                                         juce::Component& windowOwnerRef,
                                                         juce::ApplicationCommandTarget& commandTargetRef)
    : transportBar(transportBarRef), playbackEngine(playbackEngineRef), metronome(metronomeRef),
      commandManager(commandManagerRef), windowOwner(windowOwnerRef), commandTarget(commandTargetRef)
{
    metronomeVolume = MetronomeVolume::clamped(
        getAppProperties().getUserSettings()->getIntValue("metronomeVolume", MetronomeVolume::defaultPercent));
    metronome.setVolume(metronomeVolume);

    countInEnabled = getAppProperties().getUserSettings()->getBoolValue("countInEnabled", false);
    transportBar.setCountInEnabled(countInEnabled);
    countInBars = CountInBars::clamped(
        getAppProperties().getUserSettings()->getIntValue("countInBars", CountInBars::defaultBars));
    transportBar.setCountInBars(countInBars);

    transportBar.onMetronomeToggled = [this](bool enabled)
    {
        getAppProperties().getUserSettings()->setValue("metronomeEnabled", enabled);
        updateMetronomeSettings();
    };
    transportBar.onMetronomeMenuRequested = [this](juce::Component& target)
    {
        juce::PopupMenu menu;
        menu.addCommandItem(&commandManager, AppCommands::toggleCountIn);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, AppCommands::showMetronomeSettings);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&target));
    };
}

MetronomeSettingsController::~MetronomeSettingsController() = default;

void MetronomeSettingsController::toggleCountIn()
{
    setCountInEnabled(!countInEnabled);
}

bool MetronomeSettingsController::isCountInEnabled() const
{
    return countInEnabled;
}

void MetronomeSettingsController::showWindow()
{
    if (metronomeSettingsWindow == nullptr)
    {
        metronomeSettings = new MetronomeSettingsComponent();
        metronomeSettings->onMetronomeEnabledChanged = [this](bool enabled)
        {
            if (enabled != playbackEngine.isMetronomeEnabled())
                transportBar.toggleMetronome();
        };
        metronomeSettings->onVolumeChanged = [this](int percent) { setMetronomeVolume(percent); };
        metronomeSettings->onCountInEnabledChanged = [this](bool enabled) { setCountInEnabled(enabled); };
        metronomeSettings->onCountInBarsChanged = [this](int bars) { setCountInBars(bars); };
        metronomeSettingsWindow = std::make_unique<ToolWindow>("Metronome Settings", metronomeSettings, windowOwner,
                                                               commandManager, commandTarget);
    }
    updateMetronomeSettings();
    metronomeSettingsWindow->show();
}

void MetronomeSettingsController::closeWindow()
{
    metronomeSettings = nullptr;
    metronomeSettingsWindow.reset();
}

void MetronomeSettingsController::setMetronomeVolume(int percent)
{
    metronomeVolume = percent;
    metronome.setVolume(percent);
    getAppProperties().getUserSettings()->setValue("metronomeVolume", percent);
}

void MetronomeSettingsController::setCountInEnabled(bool enabled)
{
    countInEnabled = enabled;
    getAppProperties().getUserSettings()->setValue("countInEnabled", countInEnabled);
    transportBar.setCountInEnabled(countInEnabled);
    updateMetronomeSettings();
}

void MetronomeSettingsController::setCountInBars(int bars)
{
    countInBars = bars;
    getAppProperties().getUserSettings()->setValue("countInBars", countInBars);
    transportBar.setCountInBars(countInBars);
}

void MetronomeSettingsController::updateMetronomeSettings()
{
    if (metronomeSettings == nullptr)
        return;
    metronomeSettings->setMetronomeEnabled(playbackEngine.isMetronomeEnabled());
    metronomeSettings->setVolume(metronomeVolume);
    metronomeSettings->setCountInEnabled(countInEnabled);
    metronomeSettings->setCountInBars(countInBars);
}
