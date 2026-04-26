#pragma once

#include "../engine/PlaybackEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

class VstPluginHost : public PlaybackEngine::Listener
{
public:
    VstPluginHost();

    void prepare(juce::AudioProcessorGraph& graph, juce::AudioProcessorPlayer& player);
    bool loadPlugin(const juce::File& file);

    void onNoteOn(int trackIndex, const MidiNote& note) override;
    void onNoteOff(int trackIndex, const MidiNote& note) override;
    void onMidiEvent(int trackIndex, const MidiEvent& event) override;

private:
    juce::AudioPluginFormatManager formatManager;
    juce::AudioProcessorGraph* graph = nullptr;
    juce::AudioProcessorPlayer* audioPlayer = nullptr;
    juce::AudioProcessorGraph::NodeID midiInNodeId;
    juce::AudioProcessorGraph::NodeID audioOutNodeId;
    juce::AudioProcessorGraph::NodeID pluginNodeId;
};
