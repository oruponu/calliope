#pragma once

#include "audio/Metronome.h"
#include "device/MidiDeviceOutput.h"
#include "device/MidiInputReconnector.h"
#include "device/MidiThru.h"
#include "device/RecordingInput.h"
#include "device/UserDisabledMidiInputs.h"
#include "engine/PlaybackEngine.h"
#include "engine/recording/RecordingInbox.h"
#include "plugin/PluginStateChangeWatcher.h"
#include "plugin/VstPluginHost.h"
#include <functional>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

// Owns the audio device, the processor graph, the MIDI ports, the plugin host and the playback engine, and wires
// them to each other. Knows nothing of the document.
class AudioMidiHost : private juce::ChangeListener
{
public:
    // inputSourceReleased: called after the MIDI thru has let go of an input that was removed or turned off.
    explicit AudioMidiHost(std::function<void(const juce::String& deviceIdentifier)> inputSourceReleased);

    // Shutdown in two steps, so that the owner can do its own work in between. See ~MainComponent.
    void detachInputs();
    void shutDown();

    RecordingInbox& getRecordingInbox();
    PlaybackEngine& getPlaybackEngine();
    MidiDeviceOutput& getMidiOutput();
    juce::AudioDeviceManager& getAudioDeviceManager();
    Metronome& getMetronome();
    VstPluginHost& getPluginHost();
    MidiThru& getMidiThru();
    UserDisabledMidiInputs& getUserDisabledMidiInputs();

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void releaseInputSource(const juce::String& deviceIdentifier);

    std::function<void(const juce::String&)> inputSourceReleased;
    // Declared before the engine, which keeps a pointer to it, so that it outlives the engine.
    RecordingInbox recordingInbox;
    PlaybackEngine playbackEngine;
    MidiDeviceOutput midiOutput;
    juce::AudioDeviceManager audioDeviceManager;
    // Declared before the graph so that it is destroyed after every plugin the graph owns.
    PluginStateChangeWatcher pluginStateWatcher;
    juce::AudioProcessorGraph audioGraph;
    juce::AudioProcessorPlayer audioPlayer;
    Metronome metronome;
    VstPluginHost pluginHost{pluginStateWatcher};
    MidiThru midiThru{{&midiOutput, &pluginHost}};
    RecordingInput recordingInput{recordingInbox};
    MidiInputReconnector midiInputReconnector{audioDeviceManager,
                                              [this](const juce::String& id) { releaseInputSource(id); }};
    UserDisabledMidiInputs userDisabledMidiInputs{audioDeviceManager,
                                                  [this](const juce::String& id) { releaseInputSource(id); }};

    JUCE_DECLARE_NON_COPYABLE(AudioMidiHost)
};
