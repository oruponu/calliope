#include "audio/MidiThru.h"
#include <utility>

namespace
{
double now()
{
    return juce::Time::getMillisecondCounterHiRes() * 0.001;
}
} // namespace

MidiThru::MidiThru(std::vector<LiveMidiSink*> sinksToFeed) : sinks(std::move(sinksToFeed)) {}

void MidiThru::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    std::lock_guard<std::mutex> lock(mutex);
    router.handle(source->getIdentifier(), message, *this);
}

void MidiThru::setTarget(std::optional<PlaybackTrackContext> target)
{
    std::lock_guard<std::mutex> lock(mutex);
    router.setTarget(std::move(target), now(), *this);
}

void MidiThru::releaseSource(const juce::String& deviceIdentifier)
{
    std::lock_guard<std::mutex> lock(mutex);
    router.releaseSource(deviceIdentifier, now(), *this);
}

void MidiThru::sendLive(const PlaybackTrackContext& ctx, const juce::MidiMessage& message)
{
    for (auto* sink : sinks)
        sink->sendLive(ctx, message);
}
