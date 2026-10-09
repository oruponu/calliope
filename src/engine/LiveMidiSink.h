#pragma once

#include "engine/PlaybackSnapshot.h"
#include <juce_audio_basics/juce_audio_basics.h>

// Receives live input already rewritten to the target track's channel. Called on the MIDI input thread
// and, for releases, on the message thread.
class LiveMidiSink
{
public:
    virtual ~LiveMidiSink() = default;
    virtual void sendLive(const PlaybackTrackContext& ctx, const juce::MidiMessage& message) = 0;
};
