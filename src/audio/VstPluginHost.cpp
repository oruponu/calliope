#include "VstPluginHost.h"

namespace
{
class EditorWindow : public juce::DocumentWindow
{
public:
    EditorWindow(const juce::String& title, juce::AudioProcessorEditor* editor, std::function<void()> onClose)
        : DocumentWindow(title, juce::Colours::black, DocumentWindow::closeButton), closeCallback(std::move(onClose))
    {
        setUsingNativeTitleBar(true);
        setContentOwned(editor, true);
        setResizable(editor->isResizable(), false);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        if (closeCallback)
            juce::MessageManager::callAsync(closeCallback);
    }

private:
    std::function<void()> closeCallback;
};

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

bool VstPluginHost::loadPlugin(const juce::File& file)
{
    if (graph == nullptr)
        return false;

    juce::OwnedArray<juce::PluginDescription> descriptions;
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
        formatManager.getFormat(i)->findAllTypesForFile(descriptions, file.getFullPathName());

    if (descriptions.isEmpty())
        return false;

    return loadPlugin(*descriptions[0]);
}

bool VstPluginHost::loadPlugin(const juce::PluginDescription& description)
{
    return attachPlugin(0, description);
}

bool VstPluginHost::attachPlugin(int trackIndex, const juce::PluginDescription& description)
{
    if (graph == nullptr)
        return false;

    juce::String errorMessage;
    auto pluginInstance =
        formatManager.createPluginInstance(description, graph->getSampleRate(), graph->getBlockSize(), errorMessage);

    if (pluginInstance == nullptr)
        return false;

    detachPlugin(trackIndex);

    auto midiSourceProcessor = std::make_unique<MidiSourceProcessor>();
    auto* collectorPtr = &midiSourceProcessor->collector;
    auto midiSourceNode = graph->addNode(std::move(midiSourceProcessor));
    midiSourceNodes[trackIndex] = midiSourceNode->nodeID;
    midiCollectors[trackIndex] = collectorPtr;

    auto pluginNode = graph->addNode(std::move(pluginInstance));
    pluginNodes[trackIndex] = pluginNode->nodeID;

    graph->addConnection({{midiSourceNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
                          {pluginNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

    const int numOutputChannels = pluginNode->getProcessor()->getMainBusNumOutputChannels();
    const int channelsToConnect = juce::jmin(numOutputChannels, 2);
    for (int ch = 0; ch < channelsToConnect; ++ch)
        graph->addConnection({{pluginNode->nodeID, ch}, {audioOutNodeId, ch}});

    return true;
}

void VstPluginHost::detachPlugin(int trackIndex)
{
    auto it = pluginNodes.find(trackIndex);
    if (it == pluginNodes.end())
        return;

    editorWindows.erase(trackIndex);
    midiCollectors.erase(trackIndex);

    if (auto sourceIt = midiSourceNodes.find(trackIndex); sourceIt != midiSourceNodes.end())
    {
        graph->removeNode(sourceIt->second);
        midiSourceNodes.erase(sourceIt);
    }

    graph->removeNode(it->second);
    pluginNodes.erase(it);
}

void VstPluginHost::showEditor()
{
    constexpr int trackIndex = 0;

    if (auto it = editorWindows.find(trackIndex); it != editorWindows.end())
    {
        it->second->toFront(true);
        return;
    }

    if (graph == nullptr)
        return;

    auto it = pluginNodes.find(trackIndex);
    if (it == pluginNodes.end())
        return;

    auto* node = graph->getNodeForId(it->second);
    if (node == nullptr)
        return;

    auto* processor = node->getProcessor();
    if (processor == nullptr)
        return;

    auto* editor = processor->createEditorIfNeeded();
    if (editor == nullptr)
        return;

    editorWindows[trackIndex] = std::make_unique<EditorWindow>(processor->getName(), editor, [this, trackIndex]()
                                                               { editorWindows.erase(trackIndex); });
}

void VstPluginHost::onNoteOn(int trackIndex, const MidiNote& note)
{
    auto it = midiCollectors.find(trackIndex);
    if (it == midiCollectors.end())
        return;

    it->second->addMessageToQueue(
        juce::MidiMessage::noteOn(note.channel, note.noteNumber, static_cast<juce::uint8>(note.velocity)));
}

void VstPluginHost::onNoteOff(int trackIndex, const MidiNote& note)
{
    auto it = midiCollectors.find(trackIndex);
    if (it == midiCollectors.end())
        return;

    it->second->addMessageToQueue(juce::MidiMessage::noteOff(note.channel, note.noteNumber));
}

void VstPluginHost::onMidiEvent(int trackIndex, const MidiEvent& event)
{
    auto it = midiCollectors.find(trackIndex);
    if (it == midiCollectors.end())
        return;

    juce::MidiMessage msg;
    switch (event.type)
    {
    case MidiEvent::Type::ControlChange:
        msg = juce::MidiMessage::controllerEvent(event.channel, event.data1, event.data2);
        break;
    case MidiEvent::Type::ProgramChange:
        msg = juce::MidiMessage::programChange(event.channel, event.data1);
        break;
    case MidiEvent::Type::PitchBend:
        msg = juce::MidiMessage::pitchWheel(event.channel, event.data1);
        break;
    case MidiEvent::Type::ChannelPressure:
        msg = juce::MidiMessage::channelPressureChange(event.channel, event.data1);
        break;
    case MidiEvent::Type::KeyPressure:
        msg = juce::MidiMessage::aftertouchChange(event.channel, event.data1, event.data2);
        break;
    }

    it->second->addMessageToQueue(msg);
}
