#include "audio/Recorder.h"
#include "engine/PlaybackEngine.h"
#include "undo/RecordTakeAction.h"
#include "undo/UndoHistory.h"
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
    process(end.items);
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
    process(drained.items);
    const int contentBefore = builder.getContentRevision();
    const int notesBefore = builder.getNoteRevision();
    builder.releaseSource(deviceIdentifier.toStdString(), drained.position.tick);
    layerChanged(contentBefore, notesBefore);
}

const Take& Recorder::getTake() const
{
    return builder.getTake();
}

std::vector<HeldTakeNote> Recorder::getHeldNotes() const
{
    return builder.getHeldNotes();
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
    process(drained.items);
    const int contentBefore = builder.getContentRevision();
    const int notesBefore = builder.getNoteRevision();
    builder.setTarget(target, drained.position.tick);
    layerChanged(contentBefore, notesBefore);
}

void Recorder::timerCallback()
{
    process(inbox.takeItems());
    if (playbackStale && now() - lastRebuildMs >= minEventRebuildIntervalMs)
        rebuildPlayback();
}

void Recorder::process(const std::vector<InboxItem>& items)
{
    if (items.empty())
        return;
    const int contentBefore = builder.getContentRevision();
    const int notesBefore = builder.getNoteRevision();
    for (const auto& item : items)
    {
        if (const auto* input = std::get_if<RecordedInput>(&item))
            builder.handle(input->sourceId, input->message, input->position);
        else
        {
            const auto& boundary = std::get<PassBoundary>(item);
            builder.closeHeld(boundary.pass, boundary.closeTick);
        }
    }
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
    undoHistory.perform(new RecordTakeAction(&sequence, std::move(take)));
}

std::optional<TrackId> Recorder::resolveTarget() const
{
    const int index = activeTrackIndex();
    if (index < 0 || index >= sequence.getNumTracks())
        return std::nullopt;
    return sequence.getTrack(index).getId();
}
