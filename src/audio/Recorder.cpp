#include "audio/Recorder.h"
#include "engine/CountInEnd.h"
#include "engine/PlaybackEngine.h"
#include "model/TimelineGrid.h"
#include "undo/RecordTakeAction.h"
#include "undo/UndoHistory.h"
#include <optional>
#include <utility>
#include <variant>

namespace
{
constexpr int drainRateHz = 30;
// Rebuilding copies the whole song, so a stream of pitch bend must not trigger it on every drain. Finished notes
// rebuild at once instead, since the next loop pass may reach them soon.
constexpr double minEventRebuildIntervalMs = 250.0;

double now()
{
    return juce::Time::getMillisecondCounterHiRes();
}
} // namespace

Recorder::Recorder(MidiSequence& sequenceRef, UndoHistory& undoHistoryRef, PlaybackEngine& engineRef,
                   RecordingInbox& inboxRef, std::function<int()> activeTrackIndexSource)
    : sequence(sequenceRef), undoHistory(undoHistoryRef), engine(engineRef), inbox(inboxRef),
      activeTrackIndex(std::move(activeTrackIndexSource))
{
    sequence.addListener(this);
    engine.setRecordingInbox(&inbox);
    engine.setRecordingLayer(this);
    engine.onStopped = [this] { finish(); };
    triggerAsyncUpdate();
}

Recorder::~Recorder()
{
    engine.onStopped = nullptr;
    engine.setRecordingLayer(nullptr);
    engine.setRecordingInbox(nullptr);
    sequence.removeListener(this);
    cancelPendingUpdate();
    stopTimer();
}

void Recorder::start()
{
    if (recording || !engine.isPlaying())
        return;
    builder.begin(target, inbox.beginSegment(now()));
    recording = true;
    startTimerHz(drainRateHz);
    if (onRecordingChanged)
        onRecordingChanged(true);
}

void Recorder::finish()
{
    if (!recording)
        return;
    recording = false;
    stopTimer();

    auto end = inbox.endSegment(now());
    process(end.items, end.progress);
    commit(builder.end(end.position.tick));

    rebuildPlayback();
    if (onLayerChanged)
        onLayerChanged();
    if (onRecordingChanged)
        onRecordingChanged(false);
}

bool Recorder::isRecording() const
{
    return recording;
}

void Recorder::activeTrackChanged()
{
    triggerAsyncUpdate();
}

void Recorder::releaseSource(const juce::String& deviceIdentifier)
{
    if (!recording)
        return;
    const auto drained = inbox.drainAt(now());
    process(drained.items, drained.progress);
    const int contentBefore = builder.getContentRevision();
    const int notesBefore = builder.getNoteRevision();
    builder.releaseSource(deviceIdentifier.toStdString(), drained.position.tick);
    layerChanged(contentBefore, notesBefore);
}

void Recorder::setInputQuantize(std::optional<GridDivision> division)
{
    if (division == inputQuantize)
        return;
    // Input already queued was played before the change and keeps the old setting.
    if (recording)
    {
        const auto drained = inbox.drainAt(now());
        process(drained.items, drained.progress);
    }
    inputQuantize = division;
}

const Take& Recorder::getTake() const
{
    return builder.getTake();
}

std::vector<HeldTakeNote> Recorder::getHeldNotes() const
{
    return builder.getHeldNotes();
}

void Recorder::editRecordedNotes(const std::vector<RecordedNoteEdit>& edits)
{
    if (!recording || edits.empty())
        return;
    // A key released just before the edit is still queued; process it first so the note keeps its released length.
    const auto drained = inbox.drainAt(now());
    process(drained.items, drained.progress);
    const int contentBefore = builder.getContentRevision();
    const int notesBefore = builder.getNoteRevision();
    builder.editNotes(edits, drained.position.tick);
    layerChanged(contentBefore, notesBefore);
}

void Recorder::removeRecordedNotes(const std::vector<int>& ids)
{
    if (!recording || ids.empty())
        return;
    const int contentBefore = builder.getContentRevision();
    const int notesBefore = builder.getNoteRevision();
    builder.removeNotes(ids);
    layerChanged(contentBefore, notesBefore);
}

void Recorder::tracksChanged()
{
    triggerAsyncUpdate();
}

void Recorder::sequenceReset()
{
    triggerAsyncUpdate();
}

void Recorder::handleAsyncUpdate()
{
    const auto newTarget = resolveTarget();
    if (newTarget == target)
        return;
    target = newTarget;
    if (!recording)
        return;

    const auto drained = inbox.drainAt(now());
    process(drained.items, drained.progress);
    const int contentBefore = builder.getContentRevision();
    const int notesBefore = builder.getNoteRevision();
    builder.setTarget(target, drained.position.tick);
    layerChanged(contentBefore, notesBefore);
}

void Recorder::timerCallback()
{
    const auto taken = inbox.takeItems();
    process(taken.items, taken.progress);
    if (playbackStale && now() - lastRebuildMs >= minEventRebuildIntervalMs)
        rebuildPlayback();
}

void Recorder::process(const std::vector<InboxItem>& items, const PlaybackProgress& progress)
{
    const int contentBefore = builder.getContentRevision();
    const int notesBefore = builder.getNoteRevision();
    // Cheap to build since it holds the timeline by reference; built per call so its step follows the current ticks per
    // quarter note.
    std::optional<TimelineGrid> grid;
    if (inputQuantize)
        grid.emplace(sequence.getTimeline(), *inputQuantize);
    for (const auto& item : items)
    {
        if (const auto* input = std::get_if<RecordedInput>(&item))
        {
            if (input->countIn)
                builder.collectCountIn(input->sourceId, input->message);
            else
                builder.handle(input->sourceId, input->message, input->position, grid ? &*grid : nullptr, input->loop);
        }
        else if (const auto* countInEnd = std::get_if<CountInEnd>(&item))
        {
            builder.endCountIn(countInEnd->startTick, grid ? &*grid : nullptr, countInEnd->loop);
        }
        else if (const auto* boundary = std::get_if<PassBoundary>(&item))
        {
            builder.closeHeld(*boundary, progress);
        }
    }
    const bool reachedPassEnds = builder.reachPassEnds(progress.passEnds);
    const bool reached = builder.reach(progress.sent);
    if (!items.empty() || reachedPassEnds || reached)
        layerChanged(contentBefore, notesBefore);
}

void Recorder::layerChanged(int contentRevisionBefore, int noteRevisionBefore)
{
    if (builder.getNoteRevision() != noteRevisionBefore)
        rebuildPlayback();
    else if (builder.getContentRevision() != contentRevisionBefore)
        playbackStale = true;
    if (onLayerChanged)
        onLayerChanged();
}

void Recorder::rebuildPlayback()
{
    playbackStale = false;
    lastRebuildMs = now();
    engine.rebuildSnapshot();
}

void Recorder::commit(Take take)
{
    std::erase_if(take,
                  [this](const auto& entry) { return entry.second.empty() || sequence.indexOf(entry.first) < 0; });
    if (take.empty())
        return;
    undoHistory.beginNewTransaction("Record");
    auto* action = new RecordTakeAction(&sequence, std::move(take));
    if (undoHistory.perform(action) && onCommitted)
        onCommitted(action->getCommittedNotes());
}

std::optional<TrackId> Recorder::resolveTarget() const
{
    const int index = activeTrackIndex();
    if (index < 0 || index >= sequence.getNumTracks())
        return std::nullopt;
    return sequence.getTrack(index).getId();
}
