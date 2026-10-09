#include "audio/MidiInputReconnector.h"
#include <algorithm>
#include <utility>

namespace
{
bool containsIdentifier(const juce::Array<juce::MidiDeviceInfo>& devices, const juce::String& identifier)
{
    return std::ranges::any_of(devices,
                               [&](const juce::MidiDeviceInfo& device) { return device.identifier == identifier; });
}

// Falls back to the name, as AudioDeviceManager does, because the identifier can change when the device is plugged
// into another port.
const juce::MidiDeviceInfo* findReturned(const juce::Array<juce::MidiDeviceInfo>& appeared,
                                         const juce::MidiDeviceInfo& wanted)
{
    for (const auto& device : appeared)
        if (device.identifier == wanted.identifier)
            return &device;
    for (const auto& device : appeared)
        if (device.name == wanted.name)
            return &device;
    return nullptr;
}
} // namespace

MidiInputReconnector::MidiInputReconnector(juce::AudioDeviceManager& managerRef,
                                           std::function<void(const juce::String&)> onRemovedCallback)
    : manager(managerRef), onRemoved(std::move(onRemovedCallback)), knownInputs(juce::MidiInput::getAvailableDevices()),
      connection(juce::MidiDeviceListConnection::make([this] { deviceListChanged(); }))
{
}

void MidiInputReconnector::deviceListChanged()
{
    auto current = juce::MidiInput::getAvailableDevices();
    closeRemoved(current);
    reopenReturned(current);
    knownInputs = std::move(current);
}

void MidiInputReconnector::closeRemoved(const juce::Array<juce::MidiDeviceInfo>& current)
{
    for (const auto& known : knownInputs)
    {
        if (containsIdentifier(current, known.identifier))
            continue;
        if (manager.isMidiInputDeviceEnabled(known.identifier))
        {
            manager.setMidiInputDeviceEnabled(known.identifier, false);
            unplugged.add(known);
        }
        onRemoved(known.identifier);
    }
}

void MidiInputReconnector::reopenReturned(const juce::Array<juce::MidiDeviceInfo>& current)
{
    // Only devices new to the list, so that the name fallback never picks a same-named device that stayed plugged in.
    juce::Array<juce::MidiDeviceInfo> appeared;
    for (const auto& device : current)
        if (!containsIdentifier(knownInputs, device.identifier))
            appeared.add(device);

    for (int i = unplugged.size(); --i >= 0;)
    {
        if (const auto* device = findReturned(appeared, unplugged.getReference(i)))
        {
            manager.setMidiInputDeviceEnabled(device->identifier, true);
            unplugged.remove(i);
        }
    }
}
