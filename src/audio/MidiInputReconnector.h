#pragma once

#include <functional>
#include <juce_audio_devices/juce_audio_devices.h>

// AudioDeviceManager keeps an unplugged input open, so it still counts as enabled but receives nothing, even after the
// device is plugged back in. This closes such inputs and reopens them when their device comes back.
class MidiInputReconnector
{
public:
    // onRemoved runs on the message thread after the input is closed, so nothing from it can arrive after the callback.
    MidiInputReconnector(juce::AudioDeviceManager& manager,
                         std::function<void(const juce::String& identifier)> onRemovedCallback);

private:
    void deviceListChanged();
    void closeRemoved(const juce::Array<juce::MidiDeviceInfo>& current);
    void reopenReturned(const juce::Array<juce::MidiDeviceInfo>& current);

    juce::AudioDeviceManager& manager;
    std::function<void(const juce::String&)> onRemoved;
    juce::Array<juce::MidiDeviceInfo> knownInputs;
    // Inputs that were enabled when their device was unplugged.
    juce::Array<juce::MidiDeviceInfo> unplugged;
    // Declared last so that it is destroyed first and no callback reaches a half-destroyed object.
    juce::MidiDeviceListConnection connection;
};
