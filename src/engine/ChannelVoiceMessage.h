#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

inline bool isChannelVoiceMessage(const juce::MidiMessage& message)
{
    return message.isNoteOnOrOff() || message.isController() || message.isProgramChange() || message.isPitchWheel() ||
           message.isChannelPressure() || message.isAftertouch();
}
