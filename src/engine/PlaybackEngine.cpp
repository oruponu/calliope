#include "engine/PlaybackEngine.h"
#include "engine/MetronomeClicks.h"
#include <utility>

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

double ticksPerMsAt(const PlaybackSnapshot& snap, int tick)
{
    return snap.getTempoAt(tick) * snap.ticksPerQuarterNote / 60000.0;
}
} // namespace

PlaybackEngine::ScopedPause::ScopedPause(PlaybackEngine& engineRef) : engine(engineRef)
{
    if (engine.pauseDepth++ > 0)
        return;

    engine.resumeTimerAfterPause = engine.isTimerRunning();
    if (!engine.resumeTimerAfterPause)
        return;
    engine.stopTimer();
    if (auto* inbox = engine.recordingInbox.load())
        inbox->clockHalted();
}

PlaybackEngine::ScopedPause::~ScopedPause()
{
    if (--engine.pauseDepth > 0)
        return;

    if (!std::exchange(engine.resumeTimerAfterPause, false) || !engine.playing)
        return;

    engine.rebuildSnapshot();
    engine.lastSeenSnapshot.reset();
    engine.lastCallbackTimeMs = juce::Time::getMillisecondCounterHiRes();
    if (auto* inbox = engine.recordingInbox.load())
    {
        const double tick = engine.tickPosition.load();
        inbox->updateClock(engine.lastCallbackTimeMs, tick, ticksPerMsAt(*engine.currentOwner, static_cast<int>(tick)),
                           engine.currentLoop());
    }
    engine.startTimer(1);
}

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
    rebuildSnapshot();

    playing = true;
    lastSeenSnapshot.reset();
    lastCallbackTimeMs = juce::Time::getMillisecondCounterHiRes();
    if (auto* inbox = recordingInbox.load())
    {
        const int tick = static_cast<int>(tickPosition.load());
        inbox->playbackStarted(lastCallbackTimeMs, tick, ticksPerMsAt(*currentOwner, tick), currentLoop());
    }
    startTimer(1);
}

void PlaybackEngine::stop()
{
    if (!playing)
        return;

    stopTimer();
    playing = false;
    if (auto* inbox = recordingInbox.load())
        inbox->clockHalted();

    FanOut sink(listeners);
    processor.sendAllNoteOffs(sink);

    const int seek = pendingSeekTick.exchange(-1);
    if (seek >= 0)
        tickPosition.store((double)seek);

    if (onStopped)
        onStopped();
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

void PlaybackEngine::setMetronomeListener(MetronomeListener* listener)
{
    metronomeListener.store(listener);
}
void PlaybackEngine::setMetronomeEnabled(bool enabled)
{
    metronomeEnabled.store(enabled);
}
bool PlaybackEngine::isMetronomeEnabled() const
{
    return metronomeEnabled.load();
}

void PlaybackEngine::releaseActiveNotesForTrack(TrackId trackId)
{
    FanOut sink(listeners);
    processor.releaseActiveNotesForTrack(trackId, sink);
}

bool PlaybackEngine::isPaused() const
{
    return pauseDepth > 0;
}

void PlaybackEngine::setRecordingInbox(RecordingInbox* inbox)
{
    recordingInbox.store(inbox);
}

LoopRange PlaybackEngine::currentLoop() const
{
    const std::uint64_t packed = loopRange.load();
    return {loopEnabled.load(), loopStartOf(packed), loopEndOf(packed)};
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

    auto* inbox = recordingInbox.load();

    int seek = pendingSeekTick.exchange(-1);
    if (seek >= 0)
    {
        const int fromTick = static_cast<int>(tickPosition.load());
        processor.sendAllNoteOffs(sink);
        tickPosition.store((double)seek);
        processor.resetCursors(*snap, seek);
        // The seek lands at the previous callback's time; this callback then advances from there.
        if (inbox != nullptr)
            inbox->jumped(lastCallbackTimeMs, fromTick, seek, ticksPerMsAt(*snap, seek), currentLoop());
    }

    const double now = juce::Time::getMillisecondCounterHiRes();
    const double elapsedMs = now - lastCallbackTimeMs;
    lastCallbackTimeMs = now;

    const int previousTick = (int)tickPosition.load();
    const double ticksPerMs = ticksPerMsAt(*snap, previousTick);
    const double newPos = tickPosition.load() + elapsedMs * ticksPerMs;

    const std::uint64_t lr = loopRange.load();
    const int ls = loopStartOf(lr);
    const int le = loopEndOf(lr);
    if (loopEnabled.load() && le > ls && newPos >= le)
    {
        processRange(*snap, previousTick, le, sink);
        processor.sendAllNoteOffs(sink);

        double overshoot = newPos - le;
        if (overshoot > (double)(le - ls))
            overshoot = 0.0;
        const double wrapped = (double)ls + overshoot;
        tickPosition.store(wrapped);
        processor.resetCursors(*snap, ls);

        const int headEnd = (int)wrapped;
        if (headEnd > ls)
            processRange(*snap, ls, headEnd, sink);
        if (inbox != nullptr)
            inbox->loopWrapped(now, wrapped, ticksPerMsAt(*snap, headEnd), {true, ls, le});
        return;
    }

    tickPosition.store(newPos);
    const int currentTick = (int)newPos;
    if (currentTick > previousTick)
        processRange(*snap, previousTick, currentTick, sink);
    if (inbox != nullptr)
        inbox->updateClock(now, newPos, ticksPerMsAt(*snap, currentTick), currentLoop());
}

void PlaybackEngine::processRange(const PlaybackSnapshot& snap, int fromTick, int toTick, PlaybackListener& sink)
{
    processor.process(snap, fromTick, toTick, sink);

    auto* listener = metronomeListener.load();
    if (listener == nullptr || !metronomeEnabled.load())
        return;
    for (const auto& click :
         metronomeClicksInRange(snap.timeSignatureChanges, snap.ticksPerQuarterNote, fromTick, toTick))
        listener->onMetronomeClick(click.accent);
}
