#pragma once

#include "engine/CountIn.h"
#include "engine/MetronomeListener.h"
#include "engine/PlaybackClock.h"
#include "engine/PlaybackListener.h"
#include "engine/PlaybackProcessor.h"
#include "engine/PlaybackSnapshot.h"
#include "engine/RecordingInbox.h"
#include "engine/RecordingLayer.h"
#include "model/MidiSequence.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <juce_events/juce_events.h>
#include <memory>
#include <vector>

class PlaybackEngine : private juce::HighResolutionTimer
{
public:
    // While alive, no timer callback is running. Nestable; message thread only.
    class ScopedPause
    {
    public:
        explicit ScopedPause(PlaybackEngine& engineRef);
        ~ScopedPause();

        ScopedPause(const ScopedPause&) = delete;
        ScopedPause& operator=(const ScopedPause&) = delete;

    private:
        PlaybackEngine& engine;
    };

    PlaybackEngine();
    ~PlaybackEngine() override;

    void setSequence(const MidiSequence* seq);
    void rebuildSnapshot();

    void play();
    void stop();
    // Moves the playhead back to the start of its bar, clicks `bars` bars of the time signature there, whether or not
    // the metronome is on, then plays from it. A seek during the count-in moves where playback starts. Plays at once
    // from the playhead when bars is 0 or less.
    void playWithCountIn(int bars);
    bool isCountingIn() const;
    bool isPlaying() const;

    double getCurrentTick() const;
    void setPositionInTicks(int tick);

    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const;
    void setLoopRange(int startTick, int endTick);
    int getLoopStartTick() const;
    int getLoopEndTick() const;

    void addListener(PlaybackListener* listener);
    void removeListener(PlaybackListener* listener);

    void setMetronomeListener(MetronomeListener* listener);
    void setMetronomeEnabled(bool enabled);
    bool isMetronomeEnabled() const;

    void releaseActiveNotesForTrack(TrackId trackId);

    bool isPaused() const;

    // Read on the timer thread, so replace or clear it only while playback is stopped, and keep it alive until it
    // is cleared.
    void setRecordingInbox(RecordingInbox* inbox);

    // Mixed into every snapshot built after this call. Message thread.
    void setRecordingLayer(const RecordingLayer* layer);

    // Called on the message thread.
    std::function<void()> onStopped;

private:
    LoopRange currentLoop() const;

    const RecordingLayer* recordingLayer = nullptr;

    std::atomic<RecordingInbox*> recordingInbox{nullptr};

    void hiResTimerCallback() override;
    void processRange(const PlaybackSnapshot& snap, int fromTick, int toTick, PlaybackListener& sink);
    // Returns whether the count-in ended, so that the callback goes on to play from its end.
    bool advanceCountIn(const PlaybackSnapshot& snap, RecordingInbox* inbox, PlaybackListener& sink);
    void chaseAll(const PlaybackSnapshot& snap, int tick, PlaybackListener& sink);

    const MidiSequence* sequence = nullptr;

    std::atomic<bool> playing{false};
    std::atomic<double> tickPosition{0.0};
    std::atomic<int> pendingSeekTick{-1};

    std::atomic<bool> loopEnabled{false};
    std::atomic<std::uint64_t> loopRange{0};
    std::atomic<MetronomeListener*> metronomeListener{nullptr};
    std::atomic<bool> metronomeEnabled{false};

    double lastCallbackTimeMs = 0.0;
    std::shared_ptr<const PlaybackSnapshot> lastSeenSnapshot;

    std::shared_ptr<const PlaybackSnapshot> currentOwner;
    std::atomic<std::shared_ptr<const PlaybackSnapshot>> snapshot;

    std::atomic<bool> countingIn{false};
    // Set on the message thread while the timer is stopped; otherwise used by the timer thread only.
    CountIn countIn;
    double countInStartMs = 0.0;
    double countInEndMs = 0.0;
    // The tempo at the playhead when the count-in started, which the whole count-in keeps.
    double countInTicksPerMs = 0.0;
    int countInSentTick = 0;
    double pausedAtMs = 0.0;

    // Set on the message thread while the timer is stopped; otherwise used by the timer thread only.
    bool needsFullChase = false;

    int pauseDepth = 0;
    bool resumeTimerAfterPause = false;

    PlaybackProcessor processor;
    std::vector<PlaybackListener*> listeners;
};
