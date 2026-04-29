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
} // namespace

VstPluginHost::VstPluginHost()
{
    juce::addDefaultFormatsToManager(formatManager);
}

void VstPluginHost::prepare(juce::AudioProcessorGraph& g, juce::AudioProcessorPlayer& player)
{
    graph = &g;
    audioPlayer = &player;

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

    return loadPlugin(*descriptions[0]);
}

bool VstPluginHost::loadPlugin(const juce::PluginDescription& description)
{
    if (graph == nullptr)
        return false;

    juce::String errorMessage;
    auto pluginInstance =
        formatManager.createPluginInstance(description, graph->getSampleRate(), graph->getBlockSize(), errorMessage);

    if (pluginInstance == nullptr)
        return false;

    if (pluginNodeId != juce::AudioProcessorGraph::NodeID{})
    {
        editorWindow.reset();
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

void VstPluginHost::showEditor()
{
    if (editorWindow != nullptr)
    {
        editorWindow->toFront(true);
        return;
    }

    if (graph == nullptr || pluginNodeId == juce::AudioProcessorGraph::NodeID{})
        return;

    auto* node = graph->getNodeForId(pluginNodeId);
    if (node == nullptr)
        return;

    auto* processor = node->getProcessor();
    if (processor == nullptr)
        return;

    auto* editor = processor->createEditorIfNeeded();
    if (editor == nullptr)
        return;

    editorWindow = std::make_unique<EditorWindow>(processor->getName(), editor, [this]() { editorWindow.reset(); });
}

void VstPluginHost::onNoteOn(int, const MidiNote& note)
{
    if (audioPlayer == nullptr)
        return;

    audioPlayer->getMidiMessageCollector().addMessageToQueue(
        juce::MidiMessage::noteOn(note.channel, note.noteNumber, static_cast<juce::uint8>(note.velocity)));
}

void VstPluginHost::onNoteOff(int, const MidiNote& note)
{
    if (audioPlayer == nullptr)
        return;

    audioPlayer->getMidiMessageCollector().addMessageToQueue(juce::MidiMessage::noteOff(note.channel, note.noteNumber));
}

void VstPluginHost::onMidiEvent(int, const MidiEvent& event)
{
    if (audioPlayer == nullptr)
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

    audioPlayer->getMidiMessageCollector().addMessageToQueue(msg);
}
