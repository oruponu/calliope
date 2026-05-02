#pragma once

#include "../engine/PlaybackEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <unordered_map>

class VstPluginHost : public PlaybackEngine::Listener
{
public:
    VstPluginHost();

    void prepare(juce::AudioProcessorGraph& graph);
    bool loadPlugin(const juce::File& file);
    bool loadPlugin(const juce::PluginDescription& description);
    bool attachPlugin(int trackIndex, const juce::PluginDescription& description);
    void detachPlugin(int trackIndex);
    void showEditor();

    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }

    void onNoteOn(int trackIndex, const MidiNote& note) override;
    void onNoteOff(int trackIndex, const MidiNote& note) override;
    void onMidiEvent(int trackIndex, const MidiEvent& event) override;

private:
    juce::AudioPluginFormatManager formatManager;
    juce::AudioProcessorGraph* graph = nullptr;
    juce::AudioProcessorGraph::NodeID audioOutNodeId;
    std::unordered_map<int, juce::AudioProcessorGraph::NodeID> pluginNodes;
    std::unordered_map<int, juce::AudioProcessorGraph::NodeID> midiSourceNodes;
    std::unordered_map<int, juce::MidiMessageCollector*> midiCollectors;
    std::unordered_map<int, std::unique_ptr<juce::DocumentWindow>> editorWindows;
};
