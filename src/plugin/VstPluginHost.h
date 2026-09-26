#pragma once

#include "engine/PlaybackListener.h"
#include "model/TrackId.h"
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <optional>
#include <unordered_map>

class VstPluginHost : public PlaybackListener
{
public:
    VstPluginHost();

    void prepare(juce::AudioProcessorGraph& graph);
    std::optional<juce::PluginDescription> describePluginFile(const juce::File& file);
    bool attachPlugin(TrackId trackId, const juce::PluginDescription& description);
    void detachPlugin(TrackId trackId);
    void detachAllPlugins();

    juce::String getPluginName(TrackId trackId) const;
    juce::AudioProcessor* getPluginProcessor(TrackId trackId) const;

    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }

    void onNoteOn(const PlaybackTrackContext& ctx, const MidiNote& note) override;
    void onNoteOff(const PlaybackTrackContext& ctx, const MidiNote& note) override;
    void onMidiEvent(const PlaybackTrackContext& ctx, const MidiEvent& event) override;

    std::function<void(TrackId)> onPluginDetached;

private:
    struct Instance
    {
        juce::AudioProcessorGraph::NodeID pluginNode;
        juce::AudioProcessorGraph::NodeID sourceNode;
        juce::MidiMessageCollector* collector = nullptr;
    };

    juce::MidiMessageCollector* resolveCollector(const PlaybackTrackContext& ctx) const;

    juce::AudioPluginFormatManager formatManager;
    juce::AudioProcessorGraph* graph = nullptr;
    juce::AudioProcessorGraph::NodeID audioOutNodeId;
    std::unordered_map<TrackId, Instance> instances;
};
