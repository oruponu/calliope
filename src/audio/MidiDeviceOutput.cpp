#include "MidiDeviceOutput.h"
#include "../model/MidiSequence.h"
#include "../model/MidiTrack.h"

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

void MidiDeviceOutput::setSequence(const MidiSequence* seq)
{
    sequence = seq;
}

void MidiDeviceOutput::close()
{
    if (midiOutput)
    {
        reset();
        midiOutput.reset();
    }
}

void MidiDeviceOutput::reset()
{
    if (!midiOutput)
        return;

    for (int ch = 1; ch <= 16; ++ch)
    {
        midiOutput->sendMessageNow(juce::MidiMessage::allNotesOff(ch));
        midiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(ch, 121, 0));
        midiOutput->sendMessageNow(juce::MidiMessage::pitchWheel(ch, 8192));
        midiOutput->sendMessageNow(juce::MidiMessage::programChange(ch, 0));

        // RPN: Pitch Bend Sensitivity = 2 semitones, 0 cents
        midiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(ch, 101, 0));
        midiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(ch, 100, 0));
        midiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(ch, 6, 2));
        midiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(ch, 38, 0));
        // RPN Null
        midiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(ch, 101, 127));
        midiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(ch, 100, 127));
    }
}

void MidiDeviceOutput::onNoteOn(int trackIndex, const MidiNote& note)
{
    if (!midiOutput || sequence == nullptr)
        return;
    if (trackIndex < 0 || trackIndex >= sequence->getNumTracks())
        return;
    const auto& track = sequence->getTrack(trackIndex);
    if (track.getOutputDestination() != MidiTrack::OutputDestination::MidiDevice)
        return;

    midiOutput->sendMessageNow(
        juce::MidiMessage::noteOn(track.getChannel(), note.noteNumber, static_cast<juce::uint8>(note.velocity)));
}

void MidiDeviceOutput::onNoteOff(int trackIndex, const MidiNote& note)
{
    if (!midiOutput || sequence == nullptr)
        return;
    if (trackIndex < 0 || trackIndex >= sequence->getNumTracks())
        return;
    const auto& track = sequence->getTrack(trackIndex);
    if (track.getOutputDestination() != MidiTrack::OutputDestination::MidiDevice)
        return;

    midiOutput->sendMessageNow(juce::MidiMessage::noteOff(track.getChannel(), note.noteNumber));
}

void MidiDeviceOutput::onMidiEvent(int trackIndex, const MidiEvent& event)
{
    if (!midiOutput || sequence == nullptr)
        return;
    if (trackIndex < 0 || trackIndex >= sequence->getNumTracks())
        return;
    const auto& track = sequence->getTrack(trackIndex);
    if (track.getOutputDestination() != MidiTrack::OutputDestination::MidiDevice)
        return;

    const int ch = track.getChannel();
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
