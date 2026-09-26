#include "plugin/VstPluginHost.h"
#include "model/MidiTrack.h"

namespace
{
class MidiSourceProcessor : public juce::AudioProcessor
{
public:
    MidiSourceProcessor() : AudioProcessor(BusesProperties()) {}

    const juce::String getName() const override { return "MIDI Source"; }

    void prepareToPlay(double sampleRate, int) override { collector.reset(sampleRate); }
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        midi.clear();
        collector.removeNextBlockOfMessages(midi, buffer.getNumSamples());
    }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    juce::MidiMessageCollector collector;
};
} // namespace

VstPluginHost::VstPluginHost()
{
    juce::addDefaultFormatsToManager(formatManager);
}

void VstPluginHost::prepare(juce::AudioProcessorGraph& g)
{
    graph = &g;

    using IOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
    auto audioOut = graph->addNode(std::make_unique<IOProcessor>(IOProcessor::audioOutputNode));

    audioOutNodeId = audioOut->nodeID;
}

std::optional<juce::PluginDescription> VstPluginHost::describePluginFile(const juce::File& file)
{
    juce::OwnedArray<juce::PluginDescription> descriptions;
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
        formatManager.getFormat(i)->findAllTypesForFile(descriptions, file.getFullPathName());

    if (descriptions.isEmpty())
        return std::nullopt;

    return *descriptions[0];
}

bool VstPluginHost::attachPlugin(TrackId trackId, const juce::PluginDescription& description)
{
    if (graph == nullptr)
        return false;

    juce::String errorMessage;
    auto pluginInstance =
        formatManager.createPluginInstance(description, graph->getSampleRate(), graph->getBlockSize(), errorMessage);

    if (pluginInstance == nullptr)
        return false;

    detachPlugin(trackId);

    auto midiSourceProcessor = std::make_unique<MidiSourceProcessor>();
    auto* collectorPtr = &midiSourceProcessor->collector;
    auto midiSourceNode = graph->addNode(std::move(midiSourceProcessor));
    auto pluginNode = graph->addNode(std::move(pluginInstance));

    graph->addConnection({{midiSourceNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
                          {pluginNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

    const int numOutputChannels = pluginNode->getProcessor()->getMainBusNumOutputChannels();
    const int channelsToConnect = juce::jmin(numOutputChannels, 2);
    for (int ch = 0; ch < channelsToConnect; ++ch)
        graph->addConnection({{pluginNode->nodeID, ch}, {audioOutNodeId, ch}});

    instances[trackId] = Instance{pluginNode->nodeID, midiSourceNode->nodeID, collectorPtr};
    return true;
}

void VstPluginHost::detachPlugin(TrackId trackId)
{
    auto it = instances.find(trackId);
    if (it == instances.end())
        return;

    if (onPluginDetached)
        onPluginDetached(trackId);

    const Instance instance = it->second;
    instances.erase(it);
    graph->removeNode(instance.sourceNode);
    graph->removeNode(instance.pluginNode);
}

void VstPluginHost::detachAllPlugins()
{
    std::vector<TrackId> trackIds;
    trackIds.reserve(instances.size());
    for (const auto& [trackId, _] : instances)
        trackIds.push_back(trackId);
    for (TrackId trackId : trackIds)
        detachPlugin(trackId);
}

juce::AudioProcessor* VstPluginHost::getPluginProcessor(TrackId trackId) const
{
    if (graph == nullptr)
        return nullptr;

    auto it = instances.find(trackId);
    if (it == instances.end())
        return nullptr;

    auto* node = graph->getNodeForId(it->second.pluginNode);
    return node != nullptr ? node->getProcessor() : nullptr;
}

juce::String VstPluginHost::getPluginName(TrackId trackId) const
{
    if (auto* processor = getPluginProcessor(trackId))
        return processor->getName();
    return {};
}

juce::MidiMessageCollector* VstPluginHost::resolveCollector(const PlaybackTrackContext& ctx) const
{
    if (ctx.destination != MidiTrack::OutputDestination::Plugin)
        return nullptr;

    auto it = instances.find(ctx.routeTarget);
    return it != instances.end() ? it->second.collector : nullptr;
}

void VstPluginHost::onNoteOn(const PlaybackTrackContext& ctx, const MidiNote& note)
{
    auto* collector = resolveCollector(ctx);
    if (collector == nullptr)
        return;
    collector->addMessageToQueue(
        juce::MidiMessage::noteOn(ctx.channel, note.noteNumber, static_cast<juce::uint8>(note.velocity)));
}

void VstPluginHost::onNoteOff(const PlaybackTrackContext& ctx, const MidiNote& note)
{
    auto* collector = resolveCollector(ctx);
    if (collector == nullptr)
        return;
    collector->addMessageToQueue(juce::MidiMessage::noteOff(ctx.channel, note.noteNumber));
}

void VstPluginHost::onMidiEvent(const PlaybackTrackContext& ctx, const MidiEvent& event)
{
    auto* collector = resolveCollector(ctx);
    if (collector == nullptr)
        return;

    const int ch = ctx.channel;
    juce::MidiMessage msg;
    switch (event.type)
    {
    case MidiEvent::Type::ControlChange:
        msg = juce::MidiMessage::controllerEvent(ch, event.data1, event.data2);
        break;
    case MidiEvent::Type::ProgramChange:
        msg = juce::MidiMessage::programChange(ch, event.data1);
        break;
    case MidiEvent::Type::PitchBend:
        msg = juce::MidiMessage::pitchWheel(ch, event.data1);
        break;
    case MidiEvent::Type::ChannelPressure:
        msg = juce::MidiMessage::channelPressureChange(ch, event.data1);
        break;
    case MidiEvent::Type::KeyPressure:
        msg = juce::MidiMessage::aftertouchChange(ch, event.data1, event.data2);
        break;
    }

    collector->addMessageToQueue(msg);
}
