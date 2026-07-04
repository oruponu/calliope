#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct AppCommands
{
    enum ID
    {
        newFile_ = 1,
        openFile,
        saveFile_,
        quitApp,
        togglePlay,
        returnToStart,
        prevBar,
        nextBar,
        switchToEditTool,
        switchToSelectTool,
        undoAction,
        redoAction,
        cutAction,
        copyAction,
        pasteAction,
        selectAllAction,
        moveNotesUp,
        moveNotesDown,
        moveSelectionPrev,
        moveSelectionNext,
        scrollViewUp,
        scrollViewDown,
        scrollViewLeft,
        scrollViewRight,
        zoomInHorizontal,
        zoomOutHorizontal,
        zoomInVertical,
        zoomOutVertical,
        zoomReset,
        toggleLoop,
        audioSettings_
    };

    static void getAllCommands(juce::Array<juce::CommandID>& commands);
    static void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result);
};
