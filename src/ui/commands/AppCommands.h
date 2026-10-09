#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct AppCommands
{
    enum ID
    {
        newFile = 1,
        openFile,
        saveFile,
        quitApp,
        togglePlay,
        returnToStart,
        prevBar,
        nextBar,
        switchToEditTool,
        switchToSelectTool,
        undo,
        redo,
        cut,
        copy,
        paste,
        selectAll,
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
        openAudioSettings,
        saveFileAs,
        importMidi,
        exportMidi,
        toggleMetronome
    };

    static void getAllCommands(juce::Array<juce::CommandID>& commands);
    static void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result);
};
