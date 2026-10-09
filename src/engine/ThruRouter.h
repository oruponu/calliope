#pragma once

#include "engine/LiveMidiSink.h"
#include "engine/PlaybackSnapshot.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <optional>
#include <set>
#include <string>
#include <tuple>

class ThruRouter
{
public:
    void handle(const juce::String& sourceId, const juce::MidiMessage& message, LiveMidiSink& sink);

    // `now` stamps the generated releases, in seconds on the Time::getMillisecondCounterHiRes() clock
    // that MidiMessageCollector expects.
    void setTarget(std::optional<PlaybackTrackContext> newTarget, double now, LiveMidiSink& sink);
    void releaseSource(const juce::String& sourceId, double now, LiveMidiSink& sink);

private:
    // (source, input channel, note number)
    using HeldNote = std::tuple<std::string, int, int>;
    // (source, input channel, controller number)
    using HeldPedal = std::tuple<std::string, int, int>;

    void sendReleases(const std::set<int>& notes, const std::set<int>& controllers, double now,
                      LiveMidiSink& sink) const;

    std::optional<PlaybackTrackContext> target;
    // Only notes sounded on the current target; cleared whenever the target changes.
    std::set<HeldNote> heldNotes;
    // Tracked even without a target, since the pedal is physically down either way.
    std::set<HeldPedal> heldPedals;
};
