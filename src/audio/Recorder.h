#pragma once

#include "engine/RecordingInbox.h"
#include "engine/RecordingLayer.h"
#include "engine/TakeBuilder.h"
#include "model/MidiSequence.h"
#include <functional>
#include <juce_events/juce_events.h>
#include <optional>
#include <vector>

class PlaybackEngine;
class UndoHistory;

// Runs recording segments on the message thread: drains the inbox into the take, follows the active track, and
// commits each segment as one undoable "Record" step.
class Recorder : public RecordingLayer, private MidiSequence::Listener, private juce::AsyncUpdater, private juce::Timer
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

    const Take& getTake() const override;
    std::vector<HeldTakeNote> getHeldNotes() const override;

    std::function<void()> onLayerChanged;
    std::function<void(bool recording)> onRecordingChanged;

private:
    void tracksChanged() override;
    void sequenceReset() override;
    void handleAsyncUpdate() override;
    void timerCallback() override;

    void process(const std::vector<InboxItem>& items);
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
    TakeBuilder builder;
    bool recording = false;
    bool playbackStale = false;
    double lastRebuildMs = 0.0;
};
