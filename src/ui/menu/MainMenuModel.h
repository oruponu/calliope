#pragma once

#include <functional>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class MidiDeviceOutput;
class PluginCatalogController;
class TrackOutputController;
class UserDisabledMidiInputs;

class MainMenuModel : public juce::MenuBarModel
{
public:
    MainMenuModel(juce::ApplicationCommandManager& commandManager, PluginCatalogController& pluginCatalog,
                  TrackOutputController& trackOutput, MidiDeviceOutput& midiOutput,
                  juce::AudioDeviceManager& audioDeviceManager, UserDisabledMidiInputs& userDisabledMidiInputs,
                  std::function<void()> showAudioSettingsCallback);

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

private:
    enum PluginMenuItemID
    {
        loadPluginItem = 1,
        managePluginsItem
    };

    juce::PopupMenu buildPluginMenu();

    juce::ApplicationCommandManager& commandManager;
    PluginCatalogController& pluginCatalog;
    TrackOutputController& trackOutput;
    MidiDeviceOutput& midiOutput;
    juce::AudioDeviceManager& audioDeviceManager;
    UserDisabledMidiInputs& userDisabledMidiInputs;
    std::function<void()> showAudioSettings;
    juce::Array<juce::PluginDescription> pluginMenuSnapshot;

    JUCE_DECLARE_NON_COPYABLE(MainMenuModel)
};
