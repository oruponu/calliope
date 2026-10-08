#include "audio/Metronome.h"
#include "audio/MetronomeProcessor.h"
#include <memory>
#include <utility>

void Metronome::prepare(juce::AudioProcessorGraph& graph, juce::AudioProcessorGraph::NodeID audioOutNodeId)
{
    auto owned = std::make_unique<MetronomeProcessor>();
    processor = owned.get();
    processor->setGain(MetronomeVolume::gainFor(volume));

    auto node = graph.addNode(std::move(owned));
    for (int ch = 0; ch < 2; ++ch)
        graph.addConnection({{node->nodeID, ch}, {audioOutNodeId, ch}});
}

void Metronome::setVolume(int percent)
{
    volume = percent;
    if (processor != nullptr)
        processor->setGain(MetronomeVolume::gainFor(volume));
}

void Metronome::onMetronomeClick(bool accent)
{
    if (processor == nullptr)
        return;
    const int noteNumber = accent ? MetronomeProcessor::accentNoteNumber : MetronomeProcessor::plainNoteNumber;
    // No timestamp, like plugin notes: both land at the start of the next audio block, so clicks stay in step.
    processor->collector.addMessageToQueue(juce::MidiMessage::noteOn(1, noteNumber, static_cast<juce::uint8>(127)));
}
