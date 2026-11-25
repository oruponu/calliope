#include "ui/time/TimeEditLauncher.h"
#include "document/Document.h"
#include "edit/time/DeleteTime.h"
#include "edit/time/InsertSilence.h"
#include "engine/PlaybackEngine.h"
#include "session/Recorder.h"
#include "ui/time/TimeEditErrorText.h"
#include "ui/time/TimeRangeDialog.h"
#include "ui/time/TimeRangeInput.h"
#include "undo/ReplaceTimedContentsAction.h"
#include <utility>

TimeEditLauncher::TimeEditLauncher(Document& documentRef, const PlaybackEngine& playbackEngineRef,
                                   const Recorder& recorderRef, juce::Component& dialogOwnerRef,
                                   std::function<void()> clearSelectionCallback)
    : document(documentRef), playbackEngine(playbackEngineRef), recorder(recorderRef), dialogOwner(dialogOwnerRef),
      clearSelection(std::move(clearSelectionCallback))
{
}

void TimeEditLauncher::insertSilence()
{
    launchTimeEdit({"Insert Silence", "Insert"}, "Insert Silence", ::insertSilence);
}

void TimeEditLauncher::deleteTime()
{
    launchTimeEdit({"Delete Time", "Delete"}, "Delete Time", ::deleteTime);
}

void TimeEditLauncher::launchTimeEdit(
    const TimeRangeDialogTexts& texts, const juce::String& transactionName,
    std::function<std::expected<TimedContents, TimeEditError>(const TimedContents&, int start, int end)> edit)
{
    const auto& timeline = document.getSequence().getTimeline();
    const int currentTick = static_cast<int>(playbackEngine.getCurrentTick());
    TimeRangeDialog::launch(
        texts, timeline, TimeRangeInput::initialStart(currentTick, timeline),
        [this, transactionName, edit = std::move(edit)](int start, int end)
        { return applyTimeEdit(transactionName, edit(document.getSequence().getTimedContents(), start, end)); },
        dialogOwner);
}

std::optional<juce::String> TimeEditLauncher::applyTimeEdit(const juce::String& transactionName,
                                                            std::expected<TimedContents, TimeEditError> after)
{
    if (recorder.isRecording())
        return juce::String("Stop recording first.");
    if (!after)
        return timeEditErrorText(after.error());

    auto& sequence = document.getSequence();
    auto before = sequence.getTimedContents();
    if (before == *after)
        return std::nullopt;

    clearSelection();
    performReplaceTimedContents(document.getHistory(), &sequence, transactionName, std::move(before),
                                std::move(*after));
    return std::nullopt;
}
