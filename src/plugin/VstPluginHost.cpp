#include "plugin/VstPluginHost.h"
#include "model/MidiTrack.h"
#include "plugin/PluginAssignmentCodec.h"
#include "plugin/PluginSyncPlan.h"

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

VstPluginHost::~VstPluginHost()
{
    if (sequence != nullptr)
        sequence->removeListener(this);
}

void VstPluginHost::setSequence(MidiSequence* seq)
{
    if (sequence != nullptr)
        sequence->removeListener(this);
    sequence = seq;
    if (sequence != nullptr)
        sequence->addListener(this);
}

bool VstPluginHost::attachPlugin(TrackId trackId, const juce::PluginDescription& description)
{
    if (!createInstance(trackId, description, nullptr))
        return false;
    failedIds.erase(trackId);
    return true;
}

void VstPluginHost::detachPlugin(TrackId trackId)
{
    destroyInstance(trackId);
    failedIds.erase(trackId);
}

bool VstPluginHost::createInstance(TrackId trackId, const juce::PluginDescription& description,
                                   const juce::MemoryBlock* state)
{
    if (graph == nullptr)
        return false;

    juce::String errorMessage;
    auto pluginInstance =
        formatManager.createPluginInstance(description, graph->getSampleRate(), graph->getBlockSize(), errorMessage);

    if (pluginInstance == nullptr)
        return false;

    if (state != nullptr)
        pluginInstance->setStateInformation(state->getData(), static_cast<int>(state->getSize()));

    destroyInstance(trackId);

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

    instances[trackId] = Instance{pluginNode->nodeID, midiSourceNode->nodeID, collectorPtr, {}};
    return true;
}

void VstPluginHost::destroyInstance(TrackId trackId)
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

void VstPluginHost::tracksChanged()
{
    syncWithSequence();
}

void VstPluginHost::sequenceReset()
{
    std::vector<TrackId> trackIds;
    trackIds.reserve(instances.size());
    for (const auto& [trackId, _] : instances)
        trackIds.push_back(trackId);
    for (TrackId trackId : trackIds)
        destroyInstance(trackId);

    retiredStates.clear();
    failedIds.clear();
}

void VstPluginHost::syncWithSequence()
{
    if (sequence == nullptr)
        return;

    std::vector<TrackId> liveIds;
    liveIds.reserve(instances.size());
    for (const auto& [trackId, _] : instances)
        liveIds.push_back(trackId);

    const auto plan = planPluginSync(*sequence, liveIds, failedIds);

    for (TrackId trackId : plan.toRetire)
    {
        if (auto* processor = getPluginProcessor(trackId))
        {
            juce::MemoryBlock state;
            processor->getStateInformation(state);
            retiredStates.put(trackId, instances.at(trackId).owner, std::move(state));
        }
        destroyInstance(trackId);
    }

    for (TrackId trackId : plan.toDestroy)
        destroyInstance(trackId);

    for (auto& [trackId, instance] : instances)
        if (const int index = sequence->indexOf(trackId); index >= 0)
            instance.owner = sequence->getTrack(index).getPluginAssignment();

    for (TrackId trackId : plan.toCreate)
    {
        const auto assignment = sequence->getTrack(sequence->indexOf(trackId)).getPluginAssignment();
        const auto description = PluginAssignmentCodec::fromXml(assignment->descriptionXml);

        auto retired = retiredStates.take(trackId);
        std::optional<juce::MemoryBlock> state = retired;
        if (!state && !assignment->state.empty())
            state = PluginAssignmentCodec::toMemoryBlock(assignment->state);

        if (description && createInstance(trackId, *description, state ? &*state : nullptr))
        {
            instances.at(trackId).owner = assignment;
            continue;
        }

        failedIds.insert(trackId);
        if (retired)
            retiredStates.put(trackId, assignment, std::move(*retired));
    }

    retiredStates.collectExpired();
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
