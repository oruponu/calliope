#include "app/AudioMidiHost.h"
#include "settings/AppProperties.h"
#include <utility>

AudioMidiHost::AudioMidiHost(std::function<void(const juce::String&)> inputSourceReleasedCallback)
    : inputSourceReleased(std::move(inputSourceReleasedCallback))
{
    if (!midiOutput.open(getAppProperties().getUserSettings()->getValue("midiOutputDeviceId")))
        midiOutput.open();

    auto savedAudioState = getAppProperties().getUserSettings()->getXmlValue("audioDeviceState");
    audioDeviceManager.initialise(0, 2, savedAudioState.get(), true);
    audioDeviceManager.addChangeListener(this);
    audioDeviceManager.addMidiInputDeviceCallback({}, &midiThru);
    audioDeviceManager.addMidiInputDeviceCallback({}, &recordingInput);

    using IOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
    const auto audioOutNodeId = audioGraph.addNode(std::make_unique<IOProcessor>(IOProcessor::audioOutputNode))->nodeID;
    pluginHost.prepare(audioGraph, audioOutNodeId);

    audioDeviceManager.addAudioCallback(&audioPlayer);
    audioPlayer.setProcessor(&audioGraph);
    // The graph has no output channels until the player prepares it, so connections made earlier are rejected.
    metronome.prepare(audioGraph, audioOutNodeId);

    pluginHost.setPlaybackEngine(&playbackEngine);

    playbackEngine.addListener(&midiOutput);
    playbackEngine.addListener(&pluginHost);
    playbackEngine.setMetronomeListener(&metronome);
    const bool metronomeEnabled = getAppProperties().getUserSettings()->getBoolValue("metronomeEnabled", false);
    playbackEngine.setMetronomeEnabled(metronomeEnabled);
}

void AudioMidiHost::detachInputs()
{
    audioDeviceManager.removeChangeListener(this);
    audioDeviceManager.removeMidiInputDeviceCallback({}, &midiThru);
    audioDeviceManager.removeMidiInputDeviceCallback({}, &recordingInput);
}

void AudioMidiHost::shutDown()
{
    playbackEngine.setMetronomeListener(nullptr);
    playbackEngine.removeListener(&pluginHost);
    playbackEngine.removeListener(&midiOutput);
    midiOutput.close();
    audioPlayer.setProcessor(nullptr);
    audioDeviceManager.removeAudioCallback(&audioPlayer);
    audioDeviceManager.closeAudioDevice();
}

RecordingInbox& AudioMidiHost::getRecordingInbox()
{
    return recordingInbox;
}

PlaybackEngine& AudioMidiHost::getPlaybackEngine()
{
    return playbackEngine;
}

MidiDeviceOutput& AudioMidiHost::getMidiOutput()
{
    return midiOutput;
}

juce::AudioDeviceManager& AudioMidiHost::getAudioDeviceManager()
{
    return audioDeviceManager;
}

Metronome& AudioMidiHost::getMetronome()
{
    return metronome;
}

VstPluginHost& AudioMidiHost::getPluginHost()
{
    return pluginHost;
}

MidiThru& AudioMidiHost::getMidiThru()
{
    return midiThru;
}

UserDisabledMidiInputs& AudioMidiHost::getUserDisabledMidiInputs()
{
    return userDisabledMidiInputs;
}

void AudioMidiHost::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &audioDeviceManager)
    {
        if (auto xml = audioDeviceManager.createStateXml())
            getAppProperties().getUserSettings()->setValue("audioDeviceState", xml.get());
    }
}

void AudioMidiHost::releaseInputSource(const juce::String& deviceIdentifier)
{
    midiThru.releaseSource(deviceIdentifier);
    inputSourceReleased(deviceIdentifier);
}
