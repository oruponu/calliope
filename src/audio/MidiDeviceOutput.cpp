#include "MidiDeviceOutput.h"
#include "../model/MidiTrack.h"

namespace
{
void sendResetMessages(juce::MidiOutput& output)
{
    for (int ch = 1; ch <= 16; ++ch)
    {
        output.sendMessageNow(juce::MidiMessage::allNotesOff(ch));
        output.sendMessageNow(juce::MidiMessage::controllerEvent(ch, 121, 0));
        output.sendMessageNow(juce::MidiMessage::pitchWheel(ch, 8192));
        output.sendMessageNow(juce::MidiMessage::programChange(ch, 0));

        // RPN: Pitch Bend Sensitivity = 2 semitones, 0 cents
        output.sendMessageNow(juce::MidiMessage::controllerEvent(ch, 101, 0));
        output.sendMessageNow(juce::MidiMessage::controllerEvent(ch, 100, 0));
        output.sendMessageNow(juce::MidiMessage::controllerEvent(ch, 6, 2));
        output.sendMessageNow(juce::MidiMessage::controllerEvent(ch, 38, 0));
        // RPN Null
        output.sendMessageNow(juce::MidiMessage::controllerEvent(ch, 101, 127));
        output.sendMessageNow(juce::MidiMessage::controllerEvent(ch, 100, 127));
    }
}
} // namespace

MidiDeviceOutput::~MidiDeviceOutput()
{
    close();
}

bool MidiDeviceOutput::open()
{
    auto devices = juce::MidiOutput::getAvailableDevices();
    if (devices.isEmpty())
        return false;

    return open(devices[0].identifier);
}

bool MidiDeviceOutput::open(const juce::String& deviceIdentifier)
{
    close();
    std::lock_guard<std::mutex> lock(sendMutex);
    midiOutput = juce::MidiOutput::openDevice(deviceIdentifier);
    if (midiOutput != nullptr)
    {
        currentDeviceIdentifier = deviceIdentifier;
        return true;
    }
    currentDeviceIdentifier.clear();
    return false;
}

juce::String MidiDeviceOutput::getCurrentDeviceIdentifier() const
{
    return currentDeviceIdentifier;
}

void MidiDeviceOutput::close()
{
    std::lock_guard<std::mutex> lock(sendMutex);
    if (midiOutput)
    {
        sendResetMessages(*midiOutput);
        midiOutput.reset();
    }
}

void MidiDeviceOutput::reset()
{
    std::lock_guard<std::mutex> lock(sendMutex);
    if (!midiOutput)
        return;

    sendResetMessages(*midiOutput);
}

void MidiDeviceOutput::onNoteOn(const PlaybackTrackContext& ctx, const MidiNote& note)
{
    std::lock_guard<std::mutex> lock(sendMutex);
    if (!midiOutput)
        return;
    if (ctx.destination != MidiTrack::OutputDestination::MidiDevice)
        return;
    midiOutput->sendMessageNow(
        juce::MidiMessage::noteOn(ctx.channel, note.noteNumber, static_cast<juce::uint8>(note.velocity)));
}

void MidiDeviceOutput::onNoteOff(const PlaybackTrackContext& ctx, const MidiNote& note)
{
    std::lock_guard<std::mutex> lock(sendMutex);
    if (!midiOutput)
        return;
    if (ctx.destination != MidiTrack::OutputDestination::MidiDevice)
        return;
    midiOutput->sendMessageNow(juce::MidiMessage::noteOff(ctx.channel, note.noteNumber));
}

void MidiDeviceOutput::onMidiEvent(const PlaybackTrackContext& ctx, const MidiEvent& event)
{
    std::lock_guard<std::mutex> lock(sendMutex);
    if (!midiOutput)
        return;
    if (ctx.destination != MidiTrack::OutputDestination::MidiDevice)
        return;

    const int ch = ctx.channel;
    juce::MidiMessage msg;

    switch (event.type)
    {
    case MidiEvent::Type::ControlChange:
        msg = juce::MidiMessage::controllerEvent(ch, event.data1, event.data2);
        break;
    case MidiEvent::Type::ProgramChange:
        msg = juce::MidiMessage::programChange(ch, event.data1);
        break;
    case MidiEvent::Type::PitchBend:
        msg = juce::MidiMessage::pitchWheel(ch, event.data1);
        break;
    case MidiEvent::Type::ChannelPressure:
        msg = juce::MidiMessage::channelPressureChange(ch, event.data1);
        break;
    case MidiEvent::Type::KeyPressure:
        msg = juce::MidiMessage::aftertouchChange(ch, event.data1, event.data2);
        break;
    }

    midiOutput->sendMessageNow(msg);
}
