#include "PlaybackEngine.h"

namespace
{
class FanOut : public PlaybackListener
{
public:
    explicit FanOut(const std::vector<PlaybackListener*>& l) : listeners(l) {}
    void onNoteOn(const PlaybackTrackContext& c, const MidiNote& n) override
    {
        for (auto* x : listeners)
            x->onNoteOn(c, n);
    }
    void onNoteOff(const PlaybackTrackContext& c, const MidiNote& n) override
    {
        for (auto* x : listeners)
            x->onNoteOff(c, n);
    }
    void onMidiEvent(const PlaybackTrackContext& c, const MidiEvent& e) override
    {
        for (auto* x : listeners)
            x->onMidiEvent(c, e);
    }

private:
    const std::vector<PlaybackListener*>& listeners;
};
} // namespace

PlaybackEngine::PlaybackEngine() = default;

PlaybackEngine::~PlaybackEngine()
{
    stop();
}

void PlaybackEngine::setSequence(const MidiSequence* seq)
{
    sequence = seq;
    if (sequence == nullptr)
    {
        if (isTimerRunning())
            stopTimer();
        playing = false;
        FanOut sink(listeners);
        processor.sendAllNoteOffs(sink);
        snapshot.store(nullptr);
        currentOwner.reset();
        lastSeenSnapshot.reset();
        pendingSeekTick.store(-1);
        return;
    }
    rebuildSnapshot();
}

void PlaybackEngine::rebuildSnapshot()
{
    if (sequence == nullptr)
        return;
    auto fresh = std::make_shared<const PlaybackSnapshot>(PlaybackSnapshot::build(*sequence));
    currentOwner = fresh;
    snapshot.store(fresh);
}

void PlaybackEngine::play()
{
    if (sequence == nullptr || playing)
        return;
    if (!currentOwner)
        rebuildSnapshot();

    playing = true;
    lastSeenSnapshot.reset();
    lastCallbackTimeMs = juce::Time::getMillisecondCounterHiRes();
    startTimer(1);
}

bool PlaybackEngine::suspendForStructuralChange()
{
    const bool wasRunning = isTimerRunning();
    if (wasRunning)
        stopTimer();

    FanOut sink(listeners);
    processor.sendAllNoteOffs(sink);
    return wasRunning;
}

void PlaybackEngine::resumeAfterStructuralChange(bool wasRunning)
{
    rebuildSnapshot();
    if (wasRunning)
    {
        lastSeenSnapshot.reset();
        lastCallbackTimeMs = juce::Time::getMillisecondCounterHiRes();
        startTimer(1);
    }
}

void PlaybackEngine::stop()
{
    if (!playing)
        return;

    stopTimer();
    playing = false;

    FanOut sink(listeners);
    processor.sendAllNoteOffs(sink);

    const int seek = pendingSeekTick.exchange(-1);
    if (seek >= 0)
        tickPosition.store((double)seek);
}

bool PlaybackEngine::isPlaying() const
{
    return playing;
}

double PlaybackEngine::getCurrentTick() const
{
    return tickPosition.load();
}

void PlaybackEngine::setPositionInTicks(int tick)
{
    if (playing)
    {
        pendingSeekTick.store(tick);
    }
    else
    {
        FanOut sink(listeners);
        processor.sendAllNoteOffs(sink);
        tickPosition.store((double)tick);
        pendingSeekTick.store(-1);
    }
}

namespace
{
std::uint64_t packLoop(int startTick, int endTick)
{
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(startTick)) << 32) |
           static_cast<std::uint64_t>(static_cast<std::uint32_t>(endTick));
}
int loopStartOf(std::uint64_t packed)
{
    return static_cast<int>(static_cast<std::uint32_t>(packed >> 32));
}
int loopEndOf(std::uint64_t packed)
{
    return static_cast<int>(static_cast<std::uint32_t>(packed & 0xffffffffULL));
}
} // namespace

void PlaybackEngine::setLoopEnabled(bool enabled)
{
    loopEnabled.store(enabled);
}
bool PlaybackEngine::isLoopEnabled() const
{
    return loopEnabled.load();
}
void PlaybackEngine::setLoopRange(int startTick, int endTick)
{
    loopRange.store(packLoop(startTick, endTick));
}
int PlaybackEngine::getLoopStartTick() const
{
    return loopStartOf(loopRange.load());
}
int PlaybackEngine::getLoopEndTick() const
{
    return loopEndOf(loopRange.load());
}

void PlaybackEngine::addListener(PlaybackListener* listener)
{
    listeners.push_back(listener);
}
void PlaybackEngine::removeListener(PlaybackListener* listener)
{
    std::erase(listeners, listener);
}

void PlaybackEngine::releaseActiveNotesForTrack(int trackIndex)
{
    FanOut sink(listeners);
    processor.releaseActiveNotesForTrack(trackIndex, sink);
}

void PlaybackEngine::hiResTimerCallback()
{
    if (!playing)
        return;

    auto snap = snapshot.load();
    if (!snap)
        return;

    FanOut sink(listeners);

    if (snap != lastSeenSnapshot)
    {
        processor.resetCursors(*snap, (int)tickPosition.load());
        lastSeenSnapshot = snap;
    }

    int seek = pendingSeekTick.exchange(-1);
    if (seek >= 0)
    {
        processor.sendAllNoteOffs(sink);
        tickPosition.store((double)seek);
        processor.resetCursors(*snap, seek);
    }

    const double now = juce::Time::getMillisecondCounterHiRes();
    const double elapsedMs = now - lastCallbackTimeMs;
    lastCallbackTimeMs = now;

    const int previousTick = (int)tickPosition.load();
    const double ticksPerMs = (snap->getTempoAt(previousTick) * snap->ticksPerQuarterNote) / 60000.0;
    const double newPos = tickPosition.load() + elapsedMs * ticksPerMs;

    const std::uint64_t lr = loopRange.load();
    const int ls = loopStartOf(lr);
    const int le = loopEndOf(lr);
    if (loopEnabled.load() && le > ls && newPos >= le)
    {
        processor.process(*snap, previousTick, le, sink);
        processor.sendAllNoteOffs(sink);

        double overshoot = newPos - le;
        if (overshoot > (double)(le - ls))
            overshoot = 0.0;
        const double wrapped = (double)ls + overshoot;
        tickPosition.store(wrapped);
        processor.resetCursors(*snap, ls);

        const int headEnd = (int)wrapped;
        if (headEnd > ls)
            processor.process(*snap, ls, headEnd, sink);
        return;
    }

    tickPosition.store(newPos);
    const int currentTick = (int)newPos;
    if (currentTick > previousTick)
        processor.process(*snap, previousTick, currentTick, sink);
}
