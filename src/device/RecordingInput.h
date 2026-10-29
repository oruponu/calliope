#pragma once

#include "engine/RecordingInbox.h"
#include <juce_audio_devices/juce_audio_devices.h>

class RecordingInput : public juce::MidiInputCallback
{
public:
    explicit RecordingInput(RecordingInbox& inboxRef) : inbox(inboxRef) {}

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override
    {
        inbox.receive(source->getIdentifier(), message);
    }

private:
    RecordingInbox& inbox;
};
