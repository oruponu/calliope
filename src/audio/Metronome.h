#pragma once

#include "audio/MetronomeVolume.h"
#include "engine/MetronomeListener.h"
#include <juce_audio_processors/juce_audio_processors.h>

class MetronomeProcessor;

class Metronome : public MetronomeListener
{
public:
    void prepare(juce::AudioProcessorGraph& graph, juce::AudioProcessorGraph::NodeID audioOutNodeId);
    void setVolume(int percent);

    void onMetronomeClick(bool accent) override;

private:
    // Owned by the graph.
    MetronomeProcessor* processor = nullptr;
    int volume = MetronomeVolume::defaultPercent;
};
