#pragma once

#include "engine/LiveMidiSink.h"
#include "engine/ThruRouter.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <mutex>
#include <optional>
#include <vector>

class MidiThru : public juce::MidiInputCallback, private LiveMidiSink
{
public:
    explicit MidiThru(std::vector<LiveMidiSink*> sinks);

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

    void setTarget(std::optional<PlaybackTrackContext> target);
    void releaseSource(const juce::String& deviceIdentifier);

private:
    void sendLive(const PlaybackTrackContext& ctx, const juce::MidiMessage& message) override;

    std::vector<LiveMidiSink*> sinks;
    // Serialises the MIDI input thread against setTarget and releaseSource from the message thread.
    std::mutex mutex;
    ThruRouter router;
};
