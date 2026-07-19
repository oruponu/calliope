#include "ui/menu/MainMenuModel.h"
#include "AppProperties.h"
#include "audio/MidiDeviceOutput.h"
#include "ui/commands/AppCommands.h"
#include "ui/plugin/PluginManagementController.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <utility>

MainMenuModel::MainMenuModel(juce::ApplicationCommandManager& commandManagerRef,
                             PluginManagementController& pluginControllerRef, MidiDeviceOutput& midiOutputRef,
                             std::function<void()> showAudioSettingsCallback)
    : commandManager(commandManagerRef), pluginController(pluginControllerRef), midiOutput(midiOutputRef),
      showAudioSettings(std::move(showAudioSettingsCallback))
{
    setApplicationCommandManagerToWatch(&commandManager);
}

juce::StringArray MainMenuModel::getMenuBarNames()
{
    return {"File", "Edit", "View", "Plugins", "Settings"};
}

juce::PopupMenu MainMenuModel::getMenuForIndex(int menuIndex, const juce::String&)
{
    juce::PopupMenu menu;
    if (menuIndex == 0)
    {
        menu.addCommandItem(&commandManager, AppCommands::newFile_);
        menu.addCommandItem(&commandManager, AppCommands::openFile);
        menu.addCommandItem(&commandManager, AppCommands::saveFile_);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, AppCommands::quitApp);
    }
    else if (menuIndex == 1)
    {
        menu.addCommandItem(&commandManager, AppCommands::undoAction);
        menu.addCommandItem(&commandManager, AppCommands::redoAction);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, AppCommands::cutAction);
        menu.addCommandItem(&commandManager, AppCommands::copyAction);
        menu.addCommandItem(&commandManager, AppCommands::pasteAction);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, AppCommands::selectAllAction);
    }
    else if (menuIndex == 2)
    {
        menu.addCommandItem(&commandManager, AppCommands::zoomInHorizontal);
        menu.addCommandItem(&commandManager, AppCommands::zoomOutHorizontal);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, AppCommands::zoomInVertical);
        menu.addCommandItem(&commandManager, AppCommands::zoomOutVertical);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, AppCommands::zoomReset);
    }
    else if (menuIndex == 3)
    {
        menu = pluginController.buildPluginMenu();
    }
    else if (menuIndex == 4)
    {
        juce::PopupMenu::Item audioSettingsItem;
        audioSettingsItem.itemID = AppCommands::audioSettings_;
        audioSettingsItem.text = "Audio Settings...";
        audioSettingsItem.action = [this]() { showAudioSettings(); };
        menu.addItem(audioSettingsItem);

        menu.addSeparator();

        juce::PopupMenu midiOutputMenu;
        auto devices = juce::MidiOutput::getAvailableDevices();
        auto currentId = midiOutput.getCurrentDeviceIdentifier();

        if (devices.isEmpty())
        {
            midiOutputMenu.addItem(juce::PopupMenu::Item("(No devices available)").setEnabled(false));
        }
        else
        {
            for (const auto& device : devices)
            {
                bool isCurrent = (device.identifier == currentId);
                midiOutputMenu.addItem(juce::PopupMenu::Item(device.name)
                                           .setTicked(isCurrent)
                                           .setAction(
                                               [this, id = device.identifier]()
                                               {
                                                   if (midiOutput.open(id))
                                                       getAppProperties().getUserSettings()->setValue(
                                                           "midiOutputDeviceId", id);
                                               }));
            }
        }

        menu.addSubMenu("MIDI Output", midiOutputMenu);
    }
    return menu;
}

void MainMenuModel::menuItemSelected(int menuItemID, int)
{
    pluginController.handleMenuSelection(menuItemID);
}
