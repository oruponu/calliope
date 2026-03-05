#pragma once

#include "../engine/PlaybackEngine.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <memory>

class MidiDeviceOutput : public PlaybackEngine::Listener
{
public:
    ~MidiDeviceOutput() override;

    bool open();
    bool open(const juce::String& deviceIdentifier);
    void close();
    void reset();

    juce::String getCurrentDeviceIdentifier() const;

    void onNoteOn(int trackIndex, const MidiNote& note) override;
    void onNoteOff(int trackIndex, const MidiNote& note) override;
    void onMidiEvent(int trackIndex, const MidiEvent& event) override;

private:
    std::unique_ptr<juce::MidiOutput> midiOutput;
    juce::String currentDeviceIdentifier;
};
