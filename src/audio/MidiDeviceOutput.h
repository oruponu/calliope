#pragma once

#include "../engine/PlaybackEngine.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <memory>

class MidiDeviceOutput : public PlaybackEngine::Listener
{
public:
    ~MidiDeviceOutput() override;

    bool open();
    void close();

    void onNoteOn(int trackIndex, const MidiNote& note) override;
    void onNoteOff(int trackIndex, const MidiNote& note) override;

private:
    std::unique_ptr<juce::MidiOutput> midiOutput;
};
