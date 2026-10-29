#pragma once

#include "model/track/TrackId.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <unordered_map>

class ToolWindow;
class VstPluginHost;

class PluginEditorController
{
public:
    // The editor windows stay above owner and pass shortcuts on to commandTarget.
    PluginEditorController(VstPluginHost& pluginHost, juce::Component& owner,
                           juce::ApplicationCommandManager& commandManager,
                           juce::ApplicationCommandTarget& commandTarget);
    ~PluginEditorController();

    void showEditor(TrackId trackId);
    void closeEditor(TrackId trackId);
    void closeAllEditors();

private:
    VstPluginHost& pluginHost;
    juce::Component& owner;
    juce::ApplicationCommandManager& commandManager;
    juce::ApplicationCommandTarget& commandTarget;
    std::unordered_map<TrackId, std::unique_ptr<ToolWindow>> editorWindows;

    JUCE_DECLARE_NON_COPYABLE(PluginEditorController)
};
