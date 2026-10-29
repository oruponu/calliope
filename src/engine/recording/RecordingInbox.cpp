#include "engine/recording/RecordingInbox.h"
#include "model/event/ChannelVoiceStatus.h"
#include <algorithm>
#include <cmath>
#include <utility>

void RecordingInbox::playbackStarted(double timeMs, int tick, double ticksPerMs, LoopRange loop)
{
    std::lock_guard lock(mutex);
    countIn.reset();
    setAnchor(timeMs, tick, ticksPerMs, loop);
    clock.pass = ++lastAnnouncedPass;
    clock.passStartTick = tick;
    clock.passStartedByLoop = false;
}

void RecordingInbox::clockHalted()
{
    std::lock_guard lock(mutex);
    clock.ticksPerMs = 0.0;
    if (countIn)
        countIn->halted = true;
}

void RecordingInbox::jumped(double timeMs, int fromTick, int toTick, double ticksPerMs, LoopRange loop)
{
    std::lock_guard lock(mutex);
    // A wrap was announced but never taken: close that pass at the loop start, not back at fromTick.
    const int closeTick = lastAnnouncedPass > clock.pass ? clock.loop.start : fromTick;
    if (recording)
        passEnds.push_back({clock.pass, fromTick});
    setAnchor(timeMs, toTick, ticksPerMs, loop);
    clock.pass = ++lastAnnouncedPass;
    clock.passStartTick = toTick;
    clock.passStartedByLoop = false;
    if (recording)
        items.push_back(PassBoundary{.pass = clock.pass, .closeTick = closeTick, .startTick = toTick});
}

void RecordingInbox::loopWrapped(double timeMs, double tick, double ticksPerMs, LoopRange loop)
{
    std::lock_guard lock(mutex);
    if (recording)
        passEnds.push_back({clock.pass, loop.end});
    setAnchor(timeMs, tick, ticksPerMs, loop);
    const int pass = clock.pass + 1;
    if (lastAnnouncedPass < pass)
    {
        lastAnnouncedPass = pass;
        if (recording)
            items.push_back(
                PassBoundary{.pass = pass, .closeTick = loop.end, .loopWrap = true, .startTick = loop.start});
    }
    clock.pass = pass;
    clock.passStartTick = loop.start;
    clock.passStartedByLoop = true;
}

void RecordingInbox::updateClock(double timeMs, double tick, double ticksPerMs, LoopRange loop)
{
    std::lock_guard lock(mutex);
    setAnchor(timeMs, tick, ticksPerMs, loop);
}

void RecordingInbox::countInStarted(double endMs, int tick, double ticksPerMs, LoopRange loop)
{
    std::lock_guard lock(mutex);
    // Anchored at the end of the count-in, so locate() puts earlier times at the pass start.
    setAnchor(endMs, tick, ticksPerMs, loop);
    clock.pass = ++lastAnnouncedPass;
    clock.passStartTick = tick;
    clock.passStartedByLoop = false;
    countIn = CountInClock{.endMs = endMs, .ticksPerMs = ticksPerMs};
}

bool RecordingInbox::countInUpdated(int tick, double ticksPerMs, LoopRange loop)
{
    std::lock_guard lock(mutex);
    if (!countIn)
        return false;
    clock.tick = tick;
    clock.passStartTick = tick;
    clock.ticksPerMs = ticksPerMs;
    clock.loop = loop;
    countIn->ticksPerMs = ticksPerMs;
    return true;
}

void RecordingInbox::countInDelayed(double endMs)
{
    std::lock_guard lock(mutex);
    if (!countIn)
        return;
    countIn->endMs = endMs;
    countIn->halted = false;
    clock.timeMs = endMs;
    clock.ticksPerMs = countIn->ticksPerMs;
}

void RecordingInbox::countInEnded()
{
    std::lock_guard lock(mutex);
    if (countIn)
        endCountIn();
}

void RecordingInbox::receive(const juce::String& sourceId, const juce::MidiMessage& message)
{
    if (!isChannelVoiceStatus(message.getRawData()[0]))
        return;

    const double timeMs = message.getTimeStamp() * 1000.0;
    std::lock_guard lock(mutex);
    if (!recording || timeMs < segmentStartMs)
        return;

    if (countIn && !countIn->halted && timeMs >= countIn->endMs)
        endCountIn();
    if (countIn)
    {
        items.push_back(RecordedInput{.sourceId = sourceId.toStdString(),
                                      .message = message,
                                      .position = {clock.pass, clock.passStartTick},
                                      .loop = clock.loop,
                                      .countIn = true});
        return;
    }

    const auto position = locate(clock, timeMs);
    announceWrapTo(position);
    items.push_back(RecordedInput{sourceId.toStdString(), message, position, clock.loop});
}

ClockPosition RecordingInbox::beginSegment(double timeMs)
{
    std::lock_guard lock(mutex);
    items.clear();
    passEnds.clear();
    recording = true;
    segmentStartMs = timeMs;
    const auto start = locate(clock, timeMs);
    // The start may already lie past a wrap the timer has not taken. Nothing is held yet, so note the pass without
    // queuing a boundary; the timer must not announce it again.
    lastAnnouncedPass = std::max(lastAnnouncedPass, start.pass);
    return start;
}

RecordingInbox::Taken RecordingInbox::takeItems()
{
    std::lock_guard lock(mutex);
    return {std::exchange(items, {}), handOverProgress()};
}

RecordingInbox::Drained RecordingInbox::drainAt(double timeMs)
{
    std::lock_guard lock(mutex);
    const auto position = closingPosition(timeMs);
    return {position, std::exchange(items, {}), handOverProgress()};
}

RecordingInbox::Drained RecordingInbox::endSegment(double timeMs)
{
    std::lock_guard lock(mutex);
    const auto position = closingPosition(timeMs);
    recording = false;
    Drained end{position, std::exchange(items, {}), handOverProgress()};
    for (auto& item : end.items)
        if (auto* input = std::get_if<RecordedInput>(&item);
            input != nullptr && input->position.pass == end.position.pass && input->position.tick > end.position.tick)
            input->position.tick = end.position.tick;
    return end;
}

void RecordingInbox::setAnchor(double timeMs, double tick, double ticksPerMs, LoopRange loop)
{
    clock.timeMs = timeMs;
    clock.tick = tick;
    clock.ticksPerMs = ticksPerMs;
    clock.loop = loop;
}

void RecordingInbox::announceWrapTo(const ClockPosition& position)
{
    if (position.pass <= lastAnnouncedPass)
        return;
    lastAnnouncedPass = position.pass;
    items.push_back(PassBoundary{
        .pass = position.pass, .closeTick = clock.loop.end, .loopWrap = true, .startTick = clock.loop.start});
}

ClockPosition RecordingInbox::closingPosition(double timeMs)
{
    const auto position = locate(clock, timeMs);
    if (!recording)
        return position;
    announceWrapTo(position);
    // Input already announced a wrap that the clock never reached (playback stopped or paused first). Closing in the
    // pass before it would stretch the announced pass's notes back across the loop.
    if (position.pass < lastAnnouncedPass)
        return {lastAnnouncedPass, clock.loop.start};
    return position;
}

PlaybackProgress RecordingInbox::handOverProgress()
{
    return {{clock.pass, std::max(0, static_cast<int>(std::floor(clock.tick)))}, std::exchange(passEnds, {})};
}

void RecordingInbox::endCountIn()
{
    countIn.reset();
    if (recording)
        items.push_back(CountInEnd{.startTick = clock.passStartTick, .loop = clock.loop});
}
