#include "ui/window/ToolWindow.h"
#include "ui/window/OwnedWindow.h"

ToolWindow::ToolWindow(const juce::String& title, juce::Component* content, juce::Component& ownerComponent,
                       juce::ApplicationCommandManager& commands, juce::ApplicationCommandTarget& target,
                       bool resizable)
    : DialogWindow(title,
                   juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
                   true, false),
      owner(ownerComponent), commandManager(commands), commandTarget(target)
{
    setUsingNativeTitleBar(true);
    setResizable(resizable, false);
    // Added here rather than by the base constructor, where the override of getDesktopWindowStyleFlags is not yet
    // in effect. Added before sizing: on Windows the first bounds a new window gets are measured against the frame
    // of a zero-size window without its title bar, which leaves the client area short of the content.
    addToDesktop();
    setContentOwned(content, true);
    centreAroundComponent(&owner, getWidth(), getHeight());
    addKeyListener(commandManager.getKeyMappings());
}

ToolWindow::~ToolWindow()
{
    releaseFromOwner(*this);
    removeKeyListener(commandManager.getKeyMappings());
}

void ToolWindow::show()
{
    // Before showing, so that a Linux window manager sees the owner when it first maps the window.
    keepAboveOwner(*this, owner);
    setVisible(true);
    toFront(true);
}

void ToolWindow::closeButtonPressed()
{
    setVisible(false);
}

void ToolWindow::visibilityChanged()
{
    DialogWindow::visibilityChanged();
    // Covers both the close button and Escape, which hides the window without closeButtonPressed.
    if (!isVisible())
        releaseFromOwner(*this);
}

int ToolWindow::getDesktopWindowStyleFlags() const
{
    return DialogWindow::getDesktopWindowStyleFlags() & ~juce::ComponentPeer::windowAppearsOnTaskbar;
}

juce::ApplicationCommandTarget* ToolWindow::getNextCommandTarget()
{
    return &commandTarget;
}

void ToolWindow::getAllCommands(juce::Array<juce::CommandID>&) {}

void ToolWindow::getCommandInfo(juce::CommandID, juce::ApplicationCommandInfo&) {}

bool ToolWindow::perform(const InvocationInfo&)
{
    return false;
}
