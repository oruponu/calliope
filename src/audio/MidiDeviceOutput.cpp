#include "MidiDeviceOutput.h"

MidiDeviceOutput::~MidiDeviceOutput()
{
    close();
}

bool MidiDeviceOutput::open()
{
    auto devices = juce::MidiOutput::getAvailableDevices();
    if (devices.isEmpty())
        return false;

    midiOutput = juce::MidiOutput::openDevice(devices[0].identifier);
    return midiOutput != nullptr;
}

void MidiDeviceOutput::close()
{
    if (midiOutput)
    {
        for (int ch = 1; ch <= 16; ++ch)
            midiOutput->sendMessageNow(juce::MidiMessage::allNotesOff(ch));

        midiOutput.reset();
    }
}

void MidiDeviceOutput::onNoteOn(int, const MidiNote& note)
{
    if (midiOutput)
        midiOutput->sendMessageNow(
            juce::MidiMessage::noteOn(note.channel, note.noteNumber, static_cast<juce::uint8>(note.velocity)));
}

void MidiDeviceOutput::onNoteOff(int, const MidiNote& note)
{
    if (midiOutput)
        midiOutput->sendMessageNow(juce::MidiMessage::noteOff(note.channel, note.noteNumber));
}

void MidiDeviceOutput::onMidiEvent(int, const MidiEvent& event)
{
    if (!midiOutput)
        return;

    juce::MidiMessage msg;

    switch (event.type)
    {
    case MidiEvent::Type::ControlChange:
        msg = juce::MidiMessage::controllerEvent(event.channel, event.data1, event.data2);
        break;
    case MidiEvent::Type::ProgramChange:
        msg = juce::MidiMessage::programChange(event.channel, event.data1);
        break;
    case MidiEvent::Type::PitchBend:
        msg = juce::MidiMessage::pitchWheel(event.channel, event.data1);
        break;
    case MidiEvent::Type::ChannelPressure:
        msg = juce::MidiMessage::channelPressureChange(event.channel, event.data1);
        break;
    case MidiEvent::Type::KeyPressure:
        msg = juce::MidiMessage::aftertouchChange(event.channel, event.data1, event.data2);
        break;
    }

    midiOutput->sendMessageNow(msg);
}
