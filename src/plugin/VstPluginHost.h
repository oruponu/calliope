#pragma once

#include "engine/PlaybackListener.h"
#include "model/MidiSequence.h"
#include "model/PluginAssignment.h"
#include "model/TrackId.h"
#include "plugin/PluginStateChangeWatcher.h"
#include "plugin/RetiredStateStore.h"
#include <cstddef>
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class PlaybackEngine;

class VstPluginHost : public PlaybackListener, public MidiSequence::Listener
{
public:
    explicit VstPluginHost(PluginStateChangeWatcher& watcher);
    ~VstPluginHost() override;

    void prepare(juce::AudioProcessorGraph& graph);
    void setSequence(MidiSequence* sequence);
    void setPlaybackEngine(PlaybackEngine* engine);

    std::optional<juce::PluginDescription> describePluginFile(const juce::File& file);
    bool attachPlugin(TrackId trackId, const juce::PluginDescription& description);
    void detachPlugin(TrackId trackId);

    juce::String getPluginName(TrackId trackId) const;
    juce::AudioProcessor* getPluginProcessor(TrackId trackId) const;
    std::optional<std::vector<std::byte>> getPluginState(TrackId trackId) const;
    const std::unordered_set<TrackId>& getFailedTracks() const { return failedIds; }

    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }

    void onNoteOn(const PlaybackTrackContext& ctx, const MidiNote& note) override;
    void onNoteOff(const PlaybackTrackContext& ctx, const MidiNote& note) override;
    void onMidiEvent(const PlaybackTrackContext& ctx, const MidiEvent& event) override;

    void flushPendingStateChanges();

    std::function<void(TrackId)> onPluginDetached;
    std::function<void()> onPluginStateChanged;

private:
    struct Instance
    {
        juce::AudioProcessorGraph::NodeID pluginNode;
        juce::AudioProcessorGraph::NodeID sourceNode;
        juce::MidiMessageCollector* collector = nullptr;
        std::weak_ptr<const PluginAssignment> owner;
    };

    void tracksChanged() override;
    void sequenceReset() override;
    void syncWithSequence();
    bool createInstance(TrackId trackId, const juce::PluginDescription& description, const juce::MemoryBlock* state);
    void destroyInstance(TrackId trackId);
    juce::MidiMessageCollector* resolveCollector(const PlaybackTrackContext& ctx) const;

    juce::AudioPluginFormatManager formatManager;
    juce::AudioProcessorGraph* graph = nullptr;
    juce::AudioProcessorGraph::NodeID audioOutNodeId;
    MidiSequence* sequence = nullptr;
    PlaybackEngine* playbackEngine = nullptr;
    std::unordered_map<TrackId, Instance> instances;
    RetiredStateStore<juce::MemoryBlock> retiredStates;
    std::unordered_set<TrackId> failedIds;
    PluginStateChangeWatcher& stateWatcher;
};
