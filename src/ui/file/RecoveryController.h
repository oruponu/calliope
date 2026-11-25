#pragma once

#include "document/AutoSaver.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class FileOperations;
class PlaybackEngine;
class Recorder;

class RecoveryController
{
public:
    RecoveryController(Document& document, const PlaybackEngine& playbackEngine, const Recorder& recorder,
                       FileOperations& fileOperations);

    // Asks whether to recover the previous session's unsaved changes if there are any, then starts auto-saving.
    void offer();
    // For a normal quit.
    void stopAndDiscard();
    // Must be called before the main window goes, because Windows destroys owned windows along with their owner.
    void closeWindow();

private:
    bool isBusy() const;

    Document& document;
    const PlaybackEngine& playbackEngine;
    const Recorder& recorder;
    FileOperations& fileOperations;
    AutoSaver autoSaver;
    juce::ScopedMessageBox alertBox;
    std::unique_ptr<juce::AlertWindow> recoveryWindow;

    JUCE_DECLARE_WEAK_REFERENCEABLE(RecoveryController)
    JUCE_DECLARE_NON_COPYABLE(RecoveryController)
};
