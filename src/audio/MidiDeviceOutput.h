#pragma once

#include "../engine/PlaybackListener.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <memory>
#include <mutex>

class MidiDeviceOutput : public PlaybackListener
{
public:
    ~MidiDeviceOutput() override;

    bool open();
    bool open(const juce::String& deviceIdentifier);
    void close();
    void reset();

    juce::String getCurrentDeviceIdentifier() const;

    void onNoteOn(const PlaybackTrackContext& ctx, const MidiNote& note) override;
    void onNoteOff(const PlaybackTrackContext& ctx, const MidiNote& note) override;
    void onMidiEvent(const PlaybackTrackContext& ctx, const MidiEvent& event) override;

private:
    std::unique_ptr<juce::MidiOutput> midiOutput;
    juce::String currentDeviceIdentifier;
    std::mutex sendMutex;
};
