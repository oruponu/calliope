#include "VstPluginHost.h"

VstPluginHost::VstPluginHost()
{
    juce::addDefaultFormatsToManager(formatManager);
}

void VstPluginHost::prepare(juce::AudioProcessorGraph& g)
{
    graph = &g;

    using IOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
    auto midiIn = graph->addNode(std::make_unique<IOProcessor>(IOProcessor::midiInputNode));
    auto audioOut = graph->addNode(std::make_unique<IOProcessor>(IOProcessor::audioOutputNode));

    midiInNodeId = midiIn->nodeID;
    audioOutNodeId = audioOut->nodeID;
}

bool VstPluginHost::loadPlugin(const juce::File& file)
{
    if (graph == nullptr)
        return false;

    juce::OwnedArray<juce::PluginDescription> descriptions;
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
        formatManager.getFormat(i)->findAllTypesForFile(descriptions, file.getFullPathName());

    if (descriptions.isEmpty())
        return false;

    juce::String errorMessage;
    auto pluginInstance = formatManager.createPluginInstance(*descriptions[0], graph->getSampleRate(),
                                                             graph->getBlockSize(), errorMessage);

    if (pluginInstance == nullptr)
        return false;

    if (pluginNodeId != juce::AudioProcessorGraph::NodeID{})
    {
        graph->removeNode(pluginNodeId);
        pluginNodeId = {};
    }

    auto pluginNode = graph->addNode(std::move(pluginInstance));
    pluginNodeId = pluginNode->nodeID;

    graph->addConnection({{midiInNodeId, juce::AudioProcessorGraph::midiChannelIndex},
                          {pluginNodeId, juce::AudioProcessorGraph::midiChannelIndex}});

    const int numOutputChannels = pluginNode->getProcessor()->getMainBusNumOutputChannels();
    const int channelsToConnect = juce::jmin(numOutputChannels, 2);
    for (int ch = 0; ch < channelsToConnect; ++ch)
        graph->addConnection({{pluginNodeId, ch}, {audioOutNodeId, ch}});

    return true;
}

void VstPluginHost::onNoteOn(int, const MidiNote&) {}

void VstPluginHost::onNoteOff(int, const MidiNote&) {}

void VstPluginHost::onMidiEvent(int, const MidiEvent&) {}
