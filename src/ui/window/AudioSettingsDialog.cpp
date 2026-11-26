#include "ui/window/AudioSettingsDialog.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

void showAudioSettingsDialog(juce::AudioDeviceManager& deviceManager, juce::Colour background)
{
    auto* selector = new juce::AudioDeviceSelectorComponent(deviceManager, 0, 0, 2, 2, false, false, true, false);
    selector->setSize(500, 450);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(selector);
    options.dialogTitle = "Audio Settings";
    options.dialogBackgroundColour = background;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
}
