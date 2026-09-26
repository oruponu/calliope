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
    pluginHost.onPluginDetached = [this](TrackId trackId) { closeEditor(trackId); };
}

PluginEditorController::~PluginEditorController()
{
    pluginHost.onPluginDetached = nullptr;
}

void PluginEditorController::showEditor(TrackId trackId)
{
    if (auto it = editorWindows.find(trackId); it != editorWindows.end())
    {
        it->second->toFront(true);
        return;
    }

    auto* processor = pluginHost.getPluginProcessor(trackId);
    if (processor == nullptr)
        return;

    auto* editor = processor->createEditorIfNeeded();
    if (editor == nullptr)
        return;

    editorWindows[trackId] = std::make_unique<EditorWindow>(processor->getName(), editor,
                                                            [this, trackId]() { editorWindows.erase(trackId); });
}

void PluginEditorController::closeEditor(TrackId trackId)
{
    editorWindows.erase(trackId);
}
