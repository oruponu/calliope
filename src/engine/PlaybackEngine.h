#pragma once

#include "../model/MidiSequence.h"
#include "PlaybackListener.h"
#include "PlaybackProcessor.h"
#include "PlaybackSnapshot.h"
#include <atomic>
#include <cstdint>
#include <juce_events/juce_events.h>
#include <memory>
#include <vector>

class PlaybackEngine : private juce::HighResolutionTimer
{
public:
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

    void releaseActiveNotesForTrack(int trackIndex);

    bool suspendForStructuralChange();
    void resumeAfterStructuralChange(bool wasRunning);

private:
    void hiResTimerCallback() override;

    const MidiSequence* sequence = nullptr;

    std::atomic<bool> playing{false};
    std::atomic<double> tickPosition{0.0};
    std::atomic<int> pendingSeekTick{-1};

    std::atomic<bool> loopEnabled{false};
    std::atomic<std::uint64_t> loopRange{0};

    double lastCallbackTimeMs = 0.0;
    std::shared_ptr<const PlaybackSnapshot> lastSeenSnapshot;

    std::shared_ptr<const PlaybackSnapshot> currentOwner;
    std::atomic<std::shared_ptr<const PlaybackSnapshot>> snapshot;

    PlaybackProcessor processor;
    std::vector<PlaybackListener*> listeners;
};
