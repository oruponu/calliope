#pragma once

#include "engine/PlaybackListener.h"
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <unordered_map>

class VstPluginHost : public PlaybackListener
{
public:
    VstPluginHost();

    void prepare(juce::AudioProcessorGraph& graph);
    bool loadPlugin(const juce::File& file);
    bool loadPlugin(const juce::PluginDescription& description);
    bool attachPlugin(int trackIndex, const juce::PluginDescription& description);
    bool attachPlugin(int trackIndex, const juce::File& file);
    void detachPlugin(int trackIndex);
    void detachAllPlugins();
    void renumberTrackIndices(int from, int delta);

    juce::String getPluginName(int trackIndex) const;
    juce::AudioProcessor* getPluginProcessor(int trackIndex) const;

    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }

    void onNoteOn(const PlaybackTrackContext& ctx, const MidiNote& note) override;
    void onNoteOff(const PlaybackTrackContext& ctx, const MidiNote& note) override;
    void onMidiEvent(const PlaybackTrackContext& ctx, const MidiEvent& event) override;

    std::function<void(int)> onPluginDetached;
    std::function<void(int, int)> onTrackIndicesRenumbered;

private:
    juce::MidiMessageCollector* resolveCollector(const PlaybackTrackContext& ctx) const;

    juce::AudioPluginFormatManager formatManager;
    juce::AudioProcessorGraph* graph = nullptr;
    juce::AudioProcessorGraph::NodeID audioOutNodeId;
    std::unordered_map<int, juce::AudioProcessorGraph::NodeID> pluginNodes;
    std::unordered_map<int, juce::AudioProcessorGraph::NodeID> midiSourceNodes;
    std::unordered_map<int, juce::MidiMessageCollector*> midiCollectors;
};
