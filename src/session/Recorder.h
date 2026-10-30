#pragma once

#include "edit/event/NoteRef.h"
#include "engine/recording/HeldTakeNote.h"
#include "engine/recording/RecordingInbox.h"
#include "engine/recording/RecordingLayer.h"
#include "engine/recording/RecordingLayerEditor.h"
#include "engine/recording/TakeBuilder.h"
#include "model/MidiSequence.h"
#include "model/grid/GridDivision.h"
#include <functional>
#include <juce_events/juce_events.h>
#include <map>
#include <optional>
#include <vector>

class PlaybackEngine;
class UndoHistory;

// Runs recording segments on the message thread: drains the inbox into the take, follows the active track, and
// commits each segment as one undoable "Record" step.
class Recorder : public RecordingLayer,
                 public RecordingLayerEditor,
                 private MidiSequence::Listener,
                 private juce::AsyncUpdater,
                 private juce::Timer
{
public:
    Recorder(MidiSequence& sequenceRef, UndoHistory& undoHistoryRef, PlaybackEngine& engineRef,
             RecordingInbox& inboxRef, std::function<int()> activeTrackIndexSource);
    ~Recorder() override;

    Recorder(const Recorder&) = delete;
    Recorder& operator=(const Recorder&) = delete;

    // Starts a segment at the playhead. Does nothing unless playback is running.
    void start();
    // Ends the segment, if there is one, and commits what it recorded.
    void finish();
    bool isRecording() const;

    void activeTrackChanged();
    void releaseSource(const juce::String& deviceIdentifier);
    // The grid that notes played from now on start on, or nullopt to record them where they are played.
    void setInputQuantize(std::optional<GridDivision> division);

    const Take& getTake() const override;
    std::vector<HeldTakeNote> getHeldNotes() const override;

    // Ignored unless recording.
    void editRecordedNotes(const std::vector<RecordedNoteEdit>& edits) override;
    void removeRecordedNotes(const std::vector<int>& ids) override;

    std::function<void()> onLayerChanged;
    std::function<void(bool recording)> onRecordingChanged;

    // Called once the "Record" step is performed and before the layer is cleared, with where each recorded note id
    // was added.
    std::function<void(const std::map<int, NoteRef>& committed)> onCommitted;

private:
    void tracksChanged() override;
    void sequenceReset() override;
    void handleAsyncUpdate() override;
    void timerCallback() override;

    void process(const std::vector<InboxItem>& items, const PlaybackProgress& progress);
    void layerChanged(int contentRevisionBefore, int noteRevisionBefore);
    void rebuildPlayback();
    void commit(Take take);
    std::optional<TrackId> resolveTarget() const;

    MidiSequence& sequence;
    UndoHistory& undoHistory;
    PlaybackEngine& engine;
    RecordingInbox& inbox;
    std::function<int()> activeTrackIndex;
    std::optional<TrackId> target;
    std::optional<GridDivision> inputQuantize;
    TakeBuilder builder;
    bool recording = false;
    bool playbackStale = false;
    double lastRebuildMs = 0.0;
};
