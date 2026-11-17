#include "edit/time/DeleteTime.h"
#include "edit/time/AttributeMeta.h"
#include "edit/time/LineCrossings.h"
#include "edit/time/TimeSignatureBars.h"
#include "model/chase/ChaseDifference.h"
#include "model/chase/ChaseScan.h"
#include "model/controller/ControllerKind.h"
#include "model/controller/ControllerPointConversion.h"
#include "model/event/MidiEvent.h"
#include <algorithm>
#include <utility>
#include <vector>

namespace
{
struct DeletedRange
{
    int start;
    int end;

    bool removes(int tick) const { return tick >= start && tick < end; }
    int moved(int tick) const { return tick >= end ? tick - (end - start) : tick; }
};

// The last change in the range moves to the start unless one at the end replaces it, keeping its place in the list.
template <typename T> std::vector<T> deleteChanges(const std::vector<T>& changes, const DeletedRange& range)
{
    const bool changesAtEnd = std::ranges::any_of(changes, [&range](const T& c) { return c.tick == range.end; });
    const auto last = std::ranges::find_last_if(changes, [&range](const T& c) { return range.removes(c.tick); });
    bool carried = changesAtEnd || last.empty();

    std::vector<T> result;
    for (auto change : changes)
    {
        if (range.removes(change.tick))
        {
            if (!carried)
            {
                auto kept = last.front();
                kept.tick = range.start;
                result.push_back(kept);
                carried = true;
            }
            continue;
        }
        change.tick = range.moved(change.tick);
        result.push_back(change);
    }
    return result;
}

// What stays when the deletion starts at the very start: the settings at tick 0.
template <typename T> std::vector<T> deleteItems(const std::vector<T>& items, const DeletedRange& range, auto keeps)
{
    std::vector<T> result;
    for (auto item : items)
    {
        if (range.removes(item.tick) && !(range.start == 0 && item.tick == 0 && keeps(item)))
            continue;
        item.tick = range.moved(item.tick);
        result.push_back(item);
    }
    return result;
}

std::vector<MidiNote> deleteNotes(const std::vector<MidiNote>& notes, const DeletedRange& range)
{
    std::vector<MidiNote> result;
    for (auto note : notes)
    {
        if (range.removes(note.startTick))
            continue;
        if (note.startTick < range.start)
        {
            if (static_cast<long long>(note.startTick) + note.duration > range.start)
                note.duration = range.start - note.startTick;
        }
        else
        {
            note.startTick = range.moved(note.startTick);
        }
        result.push_back(note);
    }
    return result;
}

bool hasKind(const std::vector<MidiEvent>& events, const ControllerKind& kind)
{
    return std::ranges::any_of(events, [&kind](const MidiEvent& e) { return kind.matches(e); });
}

std::vector<MidiEvent> deleteChannelEvents(const std::vector<MidiEvent>& events, const DeletedRange& range)
{
    auto ordered = events;
    std::ranges::stable_sort(ordered, {}, &MidiEvent::tick);
    const auto endpoints = lineCrossings(ordered, range.start, false);
    const auto atEnd = lineCrossings(ordered, range.end, false);

    ChaseScan atStartState;
    ChaseScan atEndState;
    std::vector<MidiEvent> kept;
    std::vector<MidiEvent> moved;
    for (auto event : ordered)
    {
        if (event.tick < range.start)
        {
            kept.push_back(event);
            atStartState.apply(event);
        }
        if (event.tick < range.end)
        {
            atEndState.apply(event);
            continue;
        }
        event.tick = range.moved(event.tick);
        moved.push_back(event);
    }

    std::vector<MidiEvent> endpointPoints;
    for (const auto& crossing : endpoints)
    {
        endpointPoints.push_back(crossing.point);
        atStartState.apply(crossing.point);
    }
    for (const auto& crossing : atEnd)
        atEndState.apply(crossing.point);

    auto carry = chaseEventsToReach(atStartState.items(range.start), atEndState.items(range.start), false);
    // In point form the first point at the end holds the value before it or ends a line there, so it gives the value
    // at the start; as a step it keeps the value before the start held up to it.
    std::vector<ControllerKind> kindsAtEnd;
    for (auto& event : moved)
    {
        const auto kind = ControllerKind::of(event);
        if (event.tick != range.start || !kind || !kind->canRamp() ||
            std::ranges::find(kindsAtEnd, *kind) != kindsAtEnd.end())
            continue;
        kindsAtEnd.push_back(*kind);
        std::erase_if(carry, [&kind](const MidiEvent& e) { return kind->matches(e); });
        event.ramp = false;
    }

    std::vector<MidiEvent> lineStarts;
    for (const auto& crossing : atEnd)
    {
        const auto kind = *ControllerKind::of(crossing.point);
        if (!crossing.nextAtTick && !hasKind(endpointPoints, kind) && !hasKind(carry, kind))
        {
            // The value is already held there, but the rest of the line must not start before the start.
            auto lineStart = crossing.point;
            lineStart.tick = range.start;
            lineStart.ramp = false;
            lineStarts.push_back(lineStart);
        }
    }

    std::vector<MidiEvent> result = std::move(kept);
    result.insert(result.end(), endpointPoints.begin(), endpointPoints.end());
    result.insert(result.end(), lineStarts.begin(), lineStarts.end());
    result.insert(result.end(), carry.begin(), carry.end());
    result.insert(result.end(), moved.begin(), moved.end());
    std::ranges::stable_sort(result, {}, &MidiEvent::tick);
    return convertStepsToPoints(result);
}
} // namespace

std::expected<TimedContents, TimeEditError> deleteTime(const TimedContents& before, int start, int end)
{
    if (end <= start)
        return std::unexpected(TimeEditError::EmptyRange);

    const DeletedRange range{start, end};
    TimedContents result = before;
    result.tempoChanges = deleteChanges(before.tempoChanges, range);
    result.timeSignatureChanges = deleteChanges(before.timeSignatureChanges, range);
    result.keySignatureChanges = deleteChanges(before.keySignatureChanges, range);
    result.chordChanges = deleteChanges(before.chordChanges, range);
    result.metaEvents =
        deleteItems(before.metaEvents, range, [](const MetaEvent& m) { return isAttributeMeta(m.type); });
    for (auto& track : result.tracks)
    {
        track.notes = deleteNotes(track.notes, range);
        track.sysExEvents = deleteItems(track.sysExEvents, range, [](const SysExEvent&) { return true; });
        track.metaEvents =
            deleteItems(track.metaEvents, range, [](const MetaEvent& m) { return isAttributeMeta(m.type); });
        track.events = deleteChannelEvents(track.events, range);
    }

    if (!timeSignaturesStartBars(result.timeSignatureChanges, result.ticksPerQuarterNote))
        return std::unexpected(TimeEditError::TimeSignatureOffBar);
    return result;
}
