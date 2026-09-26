#pragma once

#include "model/TrackId.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <unordered_map>

class VstPluginHost;

class PluginEditorController
{
public:
    explicit PluginEditorController(VstPluginHost& pluginHost);
    ~PluginEditorController();

    void showEditor(TrackId trackId);
    void closeEditor(TrackId trackId);

private:
    VstPluginHost& pluginHost;
    std::unordered_map<TrackId, std::unique_ptr<juce::DocumentWindow>> editorWindows;

    JUCE_DECLARE_NON_COPYABLE(PluginEditorController)
};
