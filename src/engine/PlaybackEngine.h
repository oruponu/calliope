#pragma once

#include "engine/MetronomeListener.h"
#include "engine/PlaybackListener.h"
#include "engine/PlaybackProcessor.h"
#include "engine/PlaybackSnapshot.h"
#include "model/MidiSequence.h"
#include <atomic>
#include <cstdint>
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

private:
    void hiResTimerCallback() override;
    void processRange(const PlaybackSnapshot& snap, int fromTick, int toTick, PlaybackListener& sink);

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

    int pauseDepth = 0;
    bool resumeTimerAfterPause = false;

    PlaybackProcessor processor;
    std::vector<PlaybackListener*> listeners;
};
