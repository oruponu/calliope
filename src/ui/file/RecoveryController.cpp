#include "ui/file/RecoveryController.h"
#include "document/RecoveryLocation.h"
#include "engine/PlaybackEngine.h"
#include "io/project/RecoveryFileIO.h"
#include "session/Recorder.h"
#include "ui/file/FileOperations.h"
#include <utility>

RecoveryController::RecoveryController(Document& documentRef, const PlaybackEngine& playbackEngineRef,
                                       const Recorder& recorderRef, FileOperations& fileOperationsRef)
    : document(documentRef), playbackEngine(playbackEngineRef), recorder(recorderRef),
      fileOperations(fileOperationsRef), autoSaver(document, getRecoveryFile(), [this] { return isBusy(); })
{
    document.onDidReplaceSequence = [this] { autoSaver.songReplaced(); };
}

void RecoveryController::offer()
{
    const auto recoveryFile = getRecoveryFile();
    if (!recoveryFile.existsAsFile())
    {
        autoSaver.start();
        return;
    }

    auto loaded = RecoveryFileIO::load(recoveryFile);
    if (!loaded)
    {
        recoveryFile.deleteFile();
        alertBox = juce::AlertWindow::showScopedAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::WarningIcon)
                .withTitle("Recover Unsaved Changes")
                .withMessage("The unsaved changes from the last session could not be recovered.")
                .withButton("OK", 0),
            nullptr);
        autoSaver.start();
        return;
    }

    constexpr int recoverResult = 1;
    constexpr int discardResult = 2;
    const auto name =
        loaded->originalFile == juce::File{} ? juce::String("Untitled") : loaded->originalFile.getFileName();
    const auto savedAt = recoveryFile.getLastModificationTime().formatted("%Y-%m-%d %H:%M");
    const auto message = "Calliope did not shut down properly.\nUnsaved changes to \"" + name + "\" from " + savedAt +
                         " can be recovered.";

    // Built directly rather than from MessageBoxOptions, which would give the Escape key to Discard. Escape does
    // nothing, so that the choice is always explicit.
    recoveryWindow =
        std::make_unique<juce::AlertWindow>("Recover Unsaved Changes", message, juce::MessageBoxIconType::QuestionIcon);
    recoveryWindow->setEscapeKeyCancels(false);
    recoveryWindow->addButton("Recover", recoverResult, juce::KeyPress(juce::KeyPress::returnKey));
    recoveryWindow->addButton("Discard", discardResult);

    auto onChosen = [safeThis = juce::WeakReference<RecoveryController>(this), recoveryFile,
                     recovered = std::move(*loaded)](int result) mutable
    {
        // Destroying this controller deletes the window, which still calls back afterwards.
        if (safeThis == nullptr)
            return;
        safeThis->recoveryWindow.reset();
        if (result == discardResult)
        {
            recoveryFile.deleteFile();
        }
        else
        {
            safeThis->document.restoreRecovered(std::move(recovered));
            safeThis->fileOperations.finishOpeningProject();
        }
        safeThis->autoSaver.start();
    };
    recoveryWindow->enterModalState(true, juce::ModalCallbackFunction::create(std::move(onChosen)), false);
}

void RecoveryController::stopAndDiscard()
{
    autoSaver.stopAndDiscard();
}

void RecoveryController::closeWindow()
{
    recoveryWindow.reset();
}

bool RecoveryController::isBusy() const
{
    return playbackEngine.isPlaying() || playbackEngine.isCountingIn() || recorder.isRecording() ||
           fileOperations.isInProgress() || juce::ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown();
}
