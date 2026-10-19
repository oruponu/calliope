#pragma once

#include "ui/window/ToolWindow.h"
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>

// A tool window holding a plugin's editor. Closing it asks onClose to delete the window rather than hiding it, so
// that a closed editor does not keep running.
class PluginEditorWindow : public ToolWindow
{
public:
    PluginEditorWindow(const juce::String& title, juce::AudioProcessorEditor* editor, juce::Component& owner,
                       juce::ApplicationCommandManager& commandManager, juce::ApplicationCommandTarget& commandTarget,
                       std::function<void()> onClose)
        : ToolWindow(title, editor, owner, commandManager, commandTarget, editor->isResizable()),
          closeCallback(std::move(onClose))
    {
    }

    void closeButtonPressed() override
    {
        if (closeCallback)
            juce::MessageManager::callAsync(closeCallback);
    }

    bool escapeKeyPressed() override
    {
        closeButtonPressed();
        return true;
    }

private:
    std::function<void()> closeCallback;
};
