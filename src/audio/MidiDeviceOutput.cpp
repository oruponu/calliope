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
            juce::MidiMessage::noteOn(1, note.noteNumber, static_cast<juce::uint8>(note.velocity)));
}

void MidiDeviceOutput::onNoteOff(int, const MidiNote& note)
{
    if (midiOutput)
        midiOutput->sendMessageNow(juce::MidiMessage::noteOff(1, note.noteNumber));
}
