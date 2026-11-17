#include "edit/time/InsertSilence.h"
#include "edit/time/AttributeMeta.h"
#include "edit/time/LineCrossings.h"
#include "edit/time/TimeSignatureBars.h"
#include "model/event/MidiEvent.h"
#include <algorithm>
#include <limits>
#include <optional>
#include <vector>

namespace
{
constexpr long long intMax = std::numeric_limits<int>::max();

class InsertShift
{
public:
    InsertShift(int start, long long length) : start(start), length(length) {}

    bool failed() const { return overflow; }

    bool moves(int tick) const { return tick >= start; }

    void tick(int& value)
    {
        if (!moves(value))
            return;
        const long long shifted = value + length;
        if (shifted > intMax)
            overflow = true;
        else
            value = static_cast<int>(shifted);
    }

    void note(MidiNote& n)
    {
        if (n.startTick < start)
        {
            if (static_cast<long long>(n.startTick) + n.duration > start)
                n.duration = start - n.startTick;
            return;
        }
        if (n.startTick + length + n.duration > intMax)
            overflow = true;
        tick(n.startTick);
    }

    template <typename T> void all(std::vector<T>& items)
    {
        for (auto& item : items)
            tick(item.tick);
    }

private:
    int start;
    long long length;
    bool overflow = false;
};

// Moves every change back and, when the insertion is at the very start, leaves a copy of the one in effect at tick 0.
template <typename T> void shiftChanges(std::vector<T>& changes, InsertShift& shift, bool keepFirst)
{
    std::optional<T> atZero;
    if (keepFirst)
    {
        const auto found = std::ranges::find_last_if(changes, [](const T& c) { return c.tick == 0; });
        if (!found.empty())
            atZero = found.front();
    }
    shift.all(changes);
    if (atZero)
        changes.insert(changes.begin(), *atZero);
}

// What stays when the silence goes in at the very start: the settings at tick 0.
template <typename T> void shiftUnlessKept(std::vector<T>& items, InsertShift& shift, bool atSongStart, auto keeps)
{
    for (auto& item : items)
        if (!(atSongStart && item.tick == 0 && keeps(item)))
            shift.tick(item.tick);
    // Kept items stay at tick 0 while the ones before them in the list may have moved.
    std::ranges::stable_sort(items, {}, &T::tick);
}

std::vector<MidiEvent> shiftChannelEvents(const std::vector<MidiEvent>& events, InsertShift& shift, int start,
                                          long long length)
{
    const bool atSongStart = start == 0;
    const auto crossings = lineCrossings(events, start, atSongStart);

    std::vector<MidiEvent> kept;
    std::vector<MidiEvent> moved;
    for (auto event : events)
    {
        if (event.tick < start || (atSongStart && event.tick == 0))
        {
            kept.push_back(event);
            continue;
        }
        shift.tick(event.tick);
        moved.push_back(event);
    }

    std::vector<MidiEvent> restarts;
    for (const auto& crossing : crossings)
    {
        if (start > 0)
            kept.push_back(crossing.point);
        if (!crossing.nextAtTick)
        {
            auto restart = crossing.point;
            restart.tick = static_cast<int>(start + length);
            restarts.push_back(restart);
        }
    }

    std::vector<MidiEvent> result = std::move(kept);
    result.insert(result.end(), restarts.begin(), restarts.end());
    result.insert(result.end(), moved.begin(), moved.end());
    std::ranges::stable_sort(result, {}, &MidiEvent::tick);
    return result;
}
} // namespace

std::expected<TimedContents, TimeEditError> insertSilence(const TimedContents& before, int start, int end)
{
    if (end <= start)
        return std::unexpected(TimeEditError::EmptyRange);

    const long long length = static_cast<long long>(end) - start;
    TimedContents result = before;
    InsertShift shift(start, length);
    const bool atSongStart = start == 0;

    shiftChanges(result.tempoChanges, shift, atSongStart);
    shiftChanges(result.timeSignatureChanges, shift, atSongStart);
    shift.all(result.keySignatureChanges);
    shift.all(result.chordChanges);
    shiftUnlessKept(result.metaEvents, shift, atSongStart, [](const MetaEvent& m) { return isAttributeMeta(m.type); });
    for (auto& track : result.tracks)
    {
        for (auto& note : track.notes)
            shift.note(note);
        shiftUnlessKept(track.sysExEvents, shift, atSongStart, [](const SysExEvent&) { return true; });
        shiftUnlessKept(track.metaEvents, shift, atSongStart,
                        [](const MetaEvent& m) { return isAttributeMeta(m.type); });
        track.events = shiftChannelEvents(track.events, shift, start, length);
    }

    if (shift.failed())
        return std::unexpected(TimeEditError::TickOverflow);
    if (!timeSignaturesStartBars(result.timeSignatureChanges, result.ticksPerQuarterNote))
        return std::unexpected(TimeEditError::TimeSignatureOffBar);
    return result;
}
