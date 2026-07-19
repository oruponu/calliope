#include "ui/plugin/PluginEditorController.h"
#include "plugin/VstPluginHost.h"

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

PluginEditorController::PluginEditorController(VstPluginHost& pluginHostRef) : pluginHost(pluginHostRef)
{
    pluginHost.onPluginDetached = [this](int trackIndex) { closeEditor(trackIndex); };
    pluginHost.onTrackIndicesRenumbered = [this](int from, int) { closeEditorsFromIndex(from); };
}

PluginEditorController::~PluginEditorController()
{
    pluginHost.onPluginDetached = nullptr;
    pluginHost.onTrackIndicesRenumbered = nullptr;
}

void PluginEditorController::showEditor(int trackIndex)
{
    if (auto it = editorWindows.find(trackIndex); it != editorWindows.end())
    {
        it->second->toFront(true);
        return;
    }

    auto* processor = pluginHost.getPluginProcessor(trackIndex);
    if (processor == nullptr)
        return;

    auto* editor = processor->createEditorIfNeeded();
    if (editor == nullptr)
        return;

    editorWindows[trackIndex] = std::make_unique<EditorWindow>(processor->getName(), editor, [this, trackIndex]()
                                                               { editorWindows.erase(trackIndex); });
}

void PluginEditorController::closeEditor(int trackIndex)
{
    editorWindows.erase(trackIndex);
}

void PluginEditorController::closeEditorsFromIndex(int from)
{
    for (auto it = editorWindows.begin(); it != editorWindows.end();)
    {
        if (it->first >= from)
            it = editorWindows.erase(it);
        else
            ++it;
    }
}
