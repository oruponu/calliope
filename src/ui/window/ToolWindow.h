#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// A modeless window kept above the main window. Closing it hides it, so it reopens where it was. Shortcuts pressed
// while it has focus reach commandTarget as if the main window had focus.
class ToolWindow : public juce::DialogWindow, public juce::ApplicationCommandTarget
{
public:
    // Takes ownership of content and sizes itself to it.
    ToolWindow(const juce::String& title, juce::Component* content, juce::Component& owner,
               juce::ApplicationCommandManager& commandManager, juce::ApplicationCommandTarget& commandTarget);
    ~ToolWindow() override;

    void show();

    void closeButtonPressed() override;
    void visibilityChanged() override;
    int getDesktopWindowStyleFlags() const override;

    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

private:
    juce::Component& owner;
    juce::ApplicationCommandManager& commandManager;
    juce::ApplicationCommandTarget& commandTarget;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolWindow)
};
