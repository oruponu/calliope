#pragma once

#include "edit/time/TimeEditError.h"
#include "model/TimedContents.h"
#include "ui/time/TimeRangeDialogTexts.h"
#include <expected>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>

class Document;
class PlaybackEngine;
class Recorder;

// Opens the dialogs of Insert Silence and Delete Time, and applies the edit through the undo history.
class TimeEditLauncher
{
public:
    // clearSelection: called just before an edit is applied, to clear the selections it would leave stale.
    TimeEditLauncher(Document& document, const PlaybackEngine& playbackEngine, const Recorder& recorder,
                     juce::Component& dialogOwner, std::function<void()> clearSelection);

    void insertSilence();
    void deleteTime();

private:
    void launchTimeEdit(
        const TimeRangeDialogTexts& texts, const juce::String& transactionName,
        std::function<std::expected<TimedContents, TimeEditError>(const TimedContents&, int start, int end)> edit);
    std::optional<juce::String> applyTimeEdit(const juce::String& transactionName,
                                              std::expected<TimedContents, TimeEditError> after);

    Document& document;
    const PlaybackEngine& playbackEngine;
    const Recorder& recorder;
    juce::Component& dialogOwner;
    std::function<void()> clearSelection;

    JUCE_DECLARE_NON_COPYABLE(TimeEditLauncher)
};
