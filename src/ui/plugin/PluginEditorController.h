#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <unordered_map>

class VstPluginHost;

class PluginEditorController
{
public:
    explicit PluginEditorController(VstPluginHost& pluginHost);
    ~PluginEditorController();

    void showEditor(int trackIndex);
    void closeEditor(int trackIndex);
    void closeEditorsFromIndex(int from);

private:
    VstPluginHost& pluginHost;
    std::unordered_map<int, std::unique_ptr<juce::DocumentWindow>> editorWindows;

    JUCE_DECLARE_NON_COPYABLE(PluginEditorController)
};
