#include "engine/ThruRouter.h"
#include <algorithm>
#include <array>
#include <utility>

namespace
{
constexpr std::array holdPedalControllers{64, 66, 69};

bool isHoldPedal(int controller)
{
    return std::ranges::contains(holdPedalControllers, controller);
}

bool isChannelVoiceMessage(const juce::MidiMessage& message)
{
    return message.isNoteOnOrOff() || message.isController() || message.isProgramChange() || message.isPitchWheel() ||
           message.isChannelPressure() || message.isAftertouch();
}

juce::MidiMessage stamped(juce::MidiMessage message, double now)
{
    message.setTimeStamp(now);
    return message;
}
} // namespace

void ThruRouter::handle(const juce::String& sourceId, const juce::MidiMessage& message, LiveMidiSink& sink)
{
    if (!isChannelVoiceMessage(message))
        return;

    const auto source = sourceId.toStdString();
    const int channel = message.getChannel();

    if (message.isController() && isHoldPedal(message.getControllerNumber()))
    {
        const HeldPedal pedal{source, channel, message.getControllerNumber()};
        if (message.getControllerValue() >= 64)
            heldPedals.insert(pedal);
        else
            heldPedals.erase(pedal);
    }

    if (!target)
        return;

    if (message.isNoteOn())
        heldNotes.insert(HeldNote{source, channel, message.getNoteNumber()});
    else if (message.isNoteOff() && heldNotes.erase(HeldNote{source, channel, message.getNoteNumber()}) == 0)
        return;

    auto routed = message;
    routed.setChannel(target->channel);
    sink.sendLive(*target, routed);
}

void ThruRouter::setTarget(std::optional<PlaybackTrackContext> newTarget, double now, LiveMidiSink& sink)
{
    if (target == newTarget)
        return;

    if (target)
    {
        std::set<int> notes;
        for (const auto& held : heldNotes)
            notes.insert(std::get<2>(held));
        std::set<int> controllers;
        for (const auto& held : heldPedals)
            controllers.insert(std::get<2>(held));
        sendReleases(notes, controllers, now, sink);
    }

    heldNotes.clear();
    target = std::move(newTarget);
}

void ThruRouter::releaseSource(const juce::String& sourceId, double now, LiveMidiSink& sink)
{
    const auto source = sourceId.toStdString();

    std::set<int> notes;
    for (auto it = heldNotes.begin(); it != heldNotes.end();)
    {
        if (std::get<0>(*it) != source)
        {
            ++it;
            continue;
        }
        notes.insert(std::get<2>(*it));
        it = heldNotes.erase(it);
    }

    std::set<int> controllers;
    for (auto it = heldPedals.begin(); it != heldPedals.end();)
    {
        if (std::get<0>(*it) != source)
        {
            ++it;
            continue;
        }
        controllers.insert(std::get<2>(*it));
        it = heldPedals.erase(it);
    }

    if (target)
        sendReleases(notes, controllers, now, sink);
}

void ThruRouter::sendReleases(const std::set<int>& notes, const std::set<int>& controllers, double now,
                              LiveMidiSink& sink) const
{
    for (const int note : notes)
        sink.sendLive(*target, stamped(juce::MidiMessage::noteOff(target->channel, note), now));
    for (const int controller : controllers)
        sink.sendLive(*target, stamped(juce::MidiMessage::controllerEvent(target->channel, controller, 0), now));
}
