#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class MidiDeviceOutput;
class PluginManagementController;

class MainMenuModel : public juce::MenuBarModel
{
public:
    MainMenuModel(juce::ApplicationCommandManager& commandManager, PluginManagementController& pluginController,
                  MidiDeviceOutput& midiOutput, std::function<void()> showAudioSettingsCallback);

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

private:
    juce::ApplicationCommandManager& commandManager;
    PluginManagementController& pluginController;
    MidiDeviceOutput& midiOutput;
    std::function<void()> showAudioSettings;

    JUCE_DECLARE_NON_COPYABLE(MainMenuModel)
};
