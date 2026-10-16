#pragma once

#include "engine/PassBoundary.h"
#include "engine/PlaybackClock.h"
#include "engine/PlaybackProgress.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <mutex>
#include <string>
#include <variant>
#include <vector>

struct RecordedInput
{
    std::string sourceId;
    juce::MidiMessage message;
    ClockPosition position;
    // The clock's loop when the input was placed.
    LoopRange loop;
};

using InboxItem = std::variant<RecordedInput, PassBoundary>;

// Shared by the playback timer, the MIDI input thread and the message thread. One lock covers the clock, the
// recording flag and the queue, so an input is either queued before its segment ends or dropped, and a boundary is
// always queued before input from the pass it starts.
class RecordingInbox
{
public:
    struct Taken
    {
        std::vector<InboxItem> items;
        PlaybackProgress progress;
    };

    struct Drained
    {
        ClockPosition position;
        std::vector<InboxItem> items;
        PlaybackProgress progress;
    };

    void playbackStarted(double timeMs, int tick, double ticksPerMs, LoopRange loop);
    void clockHalted();
    void jumped(double timeMs, int fromTick, int toTick, double ticksPerMs, LoopRange loop);
    void loopWrapped(double timeMs, double tick, double ticksPerMs, LoopRange loop);
    void updateClock(double timeMs, double tick, double ticksPerMs, LoopRange loop);

    // MIDI input thread. Message timestamps are seconds on the Time::getMillisecondCounterHiRes() clock.
    void receive(const juce::String& sourceId, const juce::MidiMessage& message);

    // Message thread. drainAt and endSegment hand over the queue together with the position at timeMs, so the caller
    // can close held notes there after processing everything queued before it.
    ClockPosition beginSegment(double timeMs);
    Taken takeItems();
    Drained drainAt(double timeMs);
    Drained endSegment(double timeMs);

private:
    void setAnchor(double timeMs, double tick, double ticksPerMs, LoopRange loop);
    void announceWrapTo(const ClockPosition& position);
    ClockPosition closingPosition(double timeMs);
    PlaybackProgress handOverProgress();

    mutable std::mutex mutex;
    PlaybackClock clock;
    // The highest pass started or announced. Input that crosses the loop end announces the wrap before the timer
    // takes it, and the timer must not announce it again.
    int lastAnnouncedPass = 0;
    bool recording = false;
    double segmentStartMs = 0.0;
    std::vector<InboxItem> items;
    // Passes the clock left while recording, since the previous hand-over.
    std::vector<ClockPosition> passEnds;
};
