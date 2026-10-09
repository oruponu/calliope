#include "audio/UserDisabledMidiInputs.h"
#include <utility>

UserDisabledMidiInputs::UserDisabledMidiInputs(juce::AudioDeviceManager& managerRef,
                                               std::function<void(const juce::String&)> onTurnedOffCallback)
    : manager(managerRef), onTurnedOff(std::move(onTurnedOffCallback))
{
}

void UserDisabledMidiInputs::setEnabled(const juce::String& identifier, bool enabled)
{
    manager.setMidiInputDeviceEnabled(identifier, enabled);
    if (enabled)
    {
        disabled.removeString(identifier);
        return;
    }
    disabled.addIfNotAlreadyThere(identifier);
    onTurnedOff(identifier);
}

void UserDisabledMidiInputs::deviceListChanged()
{
    for (const auto& identifier : disabled)
    {
        if (manager.isMidiInputDeviceEnabled(identifier))
        {
            manager.setMidiInputDeviceEnabled(identifier, false);
            // Notes may have arrived while the manager had it reopened.
            onTurnedOff(identifier);
        }
    }
}
