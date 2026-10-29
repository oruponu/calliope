#pragma once

#include "engine/recording/CountInEnd.h"
#include "engine/recording/PassBoundary.h"
#include "engine/transport/PlaybackClock.h"
#include "engine/transport/PlaybackProgress.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <mutex>
#include <optional>
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
    // Played before the count-in ended. Its position is the count-in end, which may still move.
    bool countIn = false;
};

using InboxItem = std::variant<RecordedInput, PassBoundary, CountInEnd>;

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
    // Playback counts in until endMs and then starts at tick. Input before endMs is count-in input, placed at tick.
    void countInStarted(double endMs, int tick, double ticksPerMs, LoopRange loop);
    // Called by every timer callback during the count-in: the playback that follows it starts at tick, with this tempo
    // and loop. Returns false once input stamped past the end has ended the count-in.
    bool countInUpdated(int tick, double ticksPerMs, LoopRange loop);
    // The timer resumed after a pause during the count-in, which now ends at endMs.
    void countInDelayed(double endMs);
    void countInEnded();

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
    void endCountIn();

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

    struct CountInClock
    {
        double endMs = 0.0;
        // Of the playback that follows, restored when the timer resumes.
        double ticksPerMs = 0.0;
        // The timer is paused, so the end may still move later: input stays count-in input whenever it is stamped.
        bool halted = false;
    };
    // Until the end of the count-in is queued or the timer takes it.
    std::optional<CountInClock> countIn;
};
