#include "ui/commands/AppCommands.h"

void AppCommands::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.addArray({newFile_,          openFile,       saveFile_,       quitApp,           togglePlay,
                       returnToStart,     prevBar,        nextBar,         switchToEditTool,  switchToSelectTool,
                       undoAction,        redoAction,     cutAction,       copyAction,        pasteAction,
                       selectAllAction,   moveNotesUp,    moveNotesDown,   moveSelectionPrev, moveSelectionNext,
                       scrollViewUp,      scrollViewDown, scrollViewLeft,  scrollViewRight,   zoomInHorizontal,
                       zoomOutHorizontal, zoomInVertical, zoomOutVertical, zoomReset,         toggleLoop});
}

void AppCommands::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID)
    {
    case newFile_:
        result.setInfo("New", "", "File", 0);
        result.addDefaultKeypress('N', juce::ModifierKeys::commandModifier);
        break;
    case openFile:
        result.setInfo("Open...", "", "File", 0);
        result.addDefaultKeypress('O', juce::ModifierKeys::commandModifier);
        break;
    case saveFile_:
        result.setInfo("Save...", "", "File", 0);
        result.addDefaultKeypress('S', juce::ModifierKeys::commandModifier);
        break;
    case quitApp:
        result.setInfo("Exit", "", "File", 0);
        break;
    case togglePlay:
        result.setInfo("Play/Stop", "", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::spaceKey, 0);
        break;
    case returnToStart:
        result.setInfo("Return to Start", "", "Transport", 0);
        result.addDefaultKeypress(',', juce::ModifierKeys::commandModifier);
        break;
    case prevBar:
        result.setInfo("Previous Bar", "", "Transport", 0);
        result.addDefaultKeypress(',', 0);
        break;
    case nextBar:
        result.setInfo("Next Bar", "", "Transport", 0);
        result.addDefaultKeypress('.', 0);
        break;
    case switchToSelectTool:
        result.setInfo("Select Tool", "", "Tools", 0);
        result.addDefaultKeypress('1', 0);
        break;
    case switchToEditTool:
        result.setInfo("Edit Tool", "", "Tools", 0);
        result.addDefaultKeypress('2', 0);
        break;
    case undoAction:
        result.setInfo("Undo", "", "Edit", 0);
        result.addDefaultKeypress('Z', juce::ModifierKeys::commandModifier);
        break;
    case redoAction:
        result.setInfo("Redo", "", "Edit", 0);
        result.addDefaultKeypress('Y', juce::ModifierKeys::commandModifier);
        break;
    case cutAction:
        result.setInfo("Cut", "", "Edit", 0);
        result.addDefaultKeypress('X', juce::ModifierKeys::commandModifier);
        break;
    case copyAction:
        result.setInfo("Copy", "", "Edit", 0);
        result.addDefaultKeypress('C', juce::ModifierKeys::commandModifier);
        break;
    case pasteAction:
        result.setInfo("Paste", "", "Edit", 0);
        result.addDefaultKeypress('V', juce::ModifierKeys::commandModifier);
        break;
    case selectAllAction:
        result.setInfo("Select All", "", "Edit", 0);
        result.addDefaultKeypress('A', juce::ModifierKeys::commandModifier);
        break;
    case moveNotesUp:
        result.setInfo("Move Up", "", "Edit", 0);
        result.addDefaultKeypress(juce::KeyPress::upKey, 0);
        break;
    case moveNotesDown:
        result.setInfo("Move Down", "", "Edit", 0);
        result.addDefaultKeypress(juce::KeyPress::downKey, 0);
        break;
    case moveSelectionPrev:
        result.setInfo("Select Previous Note", "", "Edit", 0);
        result.addDefaultKeypress(juce::KeyPress::leftKey, 0);
        break;
    case moveSelectionNext:
        result.setInfo("Select Next Note", "", "Edit", 0);
        result.addDefaultKeypress(juce::KeyPress::rightKey, 0);
        break;
    case scrollViewUp:
        result.setInfo("Scroll Up", "", "View", 0);
        result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::commandModifier);
        break;
    case scrollViewDown:
        result.setInfo("Scroll Down", "", "View", 0);
        result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::commandModifier);
        break;
    case scrollViewLeft:
        result.setInfo("Scroll Left", "", "View", 0);
        result.addDefaultKeypress(juce::KeyPress::leftKey, juce::ModifierKeys::commandModifier);
        break;
    case scrollViewRight:
        result.setInfo("Scroll Right", "", "View", 0);
        result.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::commandModifier);
        break;
    case zoomInHorizontal:
        result.setInfo("Zoom In (Horizontal)", "", "View", 0);
        result.addDefaultKeypress('=', juce::ModifierKeys::commandModifier);
        break;
    case zoomOutHorizontal:
        result.setInfo("Zoom Out (Horizontal)", "", "View", 0);
        result.addDefaultKeypress('-', juce::ModifierKeys::commandModifier);
        break;
    case zoomInVertical:
        result.setInfo("Zoom In (Vertical)", "", "View", 0);
        result.addDefaultKeypress('=', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
        break;
    case zoomOutVertical:
        result.setInfo("Zoom Out (Vertical)", "", "View", 0);
        result.addDefaultKeypress('-', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
        break;
    case zoomReset:
        result.setInfo("Reset Zoom", "", "View", 0);
        result.addDefaultKeypress('0', juce::ModifierKeys::commandModifier);
        break;
    case toggleLoop:
        result.setInfo("Toggle Loop", "", "Transport", 0);
        result.addDefaultKeypress('/', 0);
        break;
    default:
        break;
    }
}
