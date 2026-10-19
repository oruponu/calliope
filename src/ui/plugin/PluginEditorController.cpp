#include "ui/plugin/PluginEditorController.h"
#include "plugin/VstPluginHost.h"
#include "ui/plugin/PluginEditorWindow.h"

PluginEditorController::PluginEditorController(VstPluginHost& pluginHostRef, juce::Component& ownerComponent,
                                               juce::ApplicationCommandManager& commands,
                                               juce::ApplicationCommandTarget& target)
    : pluginHost(pluginHostRef), owner(ownerComponent), commandManager(commands), commandTarget(target)
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
        it->second->show();
        return;
    }

    auto* processor = pluginHost.getPluginProcessor(trackId);
    if (processor == nullptr)
        return;

    auto* editor = processor->createEditorIfNeeded();
    if (editor == nullptr)
        return;

    auto window = std::make_unique<PluginEditorWindow>(processor->getName(), editor, owner, commandManager,
                                                       commandTarget, [this, trackId]() { closeEditor(trackId); });
    window->show();
    editorWindows[trackId] = std::move(window);
}

void PluginEditorController::closeEditor(TrackId trackId)
{
    editorWindows.erase(trackId);
}

void PluginEditorController::closeAllEditors()
{
    editorWindows.clear();
}
