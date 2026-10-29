#pragma once

#include <functional>
#include <juce_audio_devices/juce_audio_devices.h>

// AudioDeviceManager re-enables every input from the state it was initialised with whenever the device list
// changes, so inputs the user has turned off since then are turned off again here.
class UserDisabledMidiInputs
{
public:
    // onTurnedOff runs after an input is closed, so nothing from it can arrive after the callback.
    UserDisabledMidiInputs(juce::AudioDeviceManager& manager,
                           std::function<void(const juce::String& identifier)> onTurnedOff);

    void setEnabled(const juce::String& identifier, bool enabled);

private:
    void deviceListChanged();

    juce::AudioDeviceManager& manager;
    std::function<void(const juce::String&)> onTurnedOff;
    juce::StringArray disabled;
    // Registered after the manager's own connection, so this runs after it has reopened its inputs.
    juce::MidiDeviceListConnection connection = juce::MidiDeviceListConnection::make([this] { deviceListChanged(); });
};
