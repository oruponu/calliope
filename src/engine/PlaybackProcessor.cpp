#include "PlaybackProcessor.h"
#include <algorithm>

void PlaybackProcessor::resetCursors(const PlaybackSnapshot& snap, int tick)
{
    noteCursor = (std::size_t)(std::lower_bound(snap.notes.begin(), snap.notes.end(), tick,
                                                [](const ScheduledNote& n, int t) { return n.note.startTick < t; }) -
                               snap.notes.begin());
    eventCursor = (std::size_t)(std::lower_bound(snap.events.begin(), snap.events.end(), tick,
                                                 [](const ScheduledEvent& e, int t) { return e.event.tick < t; }) -
                                snap.events.begin());
}

void PlaybackProcessor::offExpired(int toTick, PlaybackListener& sink)
{
    std::vector<ScheduledNote> expired;
    {
        std::lock_guard<std::mutex> lock(activeNotesMutex);
        auto it = std::stable_partition(activeNotes.begin(), activeNotes.end(),
                                        [&](const ScheduledNote& a) { return a.note.endTick() > toTick; });
        expired.assign(it, activeNotes.end());
        activeNotes.erase(it, activeNotes.end());
    }
    for (const auto& a : expired)
        sink.onNoteOff(a.ctx, a.note);
}

void PlaybackProcessor::process(const PlaybackSnapshot& snap, int fromTick, int toTick, PlaybackListener& sink)
{
    offExpired(toTick, sink);

    while (eventCursor < snap.events.size() && snap.events[eventCursor].event.tick < fromTick)
        ++eventCursor;
    while (eventCursor < snap.events.size() && snap.events[eventCursor].event.tick < toTick)
    {
        const auto& se = snap.events[eventCursor];
        sink.onMidiEvent(se.ctx, se.event);
        ++eventCursor;
    }

    while (noteCursor < snap.notes.size() && snap.notes[noteCursor].note.startTick < fromTick)
        ++noteCursor;
    std::vector<ScheduledNote> started;
    while (noteCursor < snap.notes.size() && snap.notes[noteCursor].note.startTick < toTick)
    {
        started.push_back(snap.notes[noteCursor]);
        ++noteCursor;
    }
    if (!started.empty())
    {
        std::lock_guard<std::mutex> lock(activeNotesMutex);
        for (const auto& s : started)
            activeNotes.push_back(s);
        for (const auto& s : started)
            sink.onNoteOn(s.ctx, s.note);
    }

    offExpired(toTick, sink);
}

void PlaybackProcessor::sendAllNoteOffs(PlaybackListener& sink)
{
    std::vector<ScheduledNote> toOff;
    {
        std::lock_guard<std::mutex> lock(activeNotesMutex);
        toOff.swap(activeNotes);
    }
    for (const auto& a : toOff)
        sink.onNoteOff(a.ctx, a.note);
}

void PlaybackProcessor::releaseActiveNotesForTrack(int trackIndex, PlaybackListener& sink)
{
    std::lock_guard<std::mutex> lock(activeNotesMutex);
    auto it = std::stable_partition(activeNotes.begin(), activeNotes.end(),
                                    [&](const ScheduledNote& a) { return a.ctx.trackIndex != trackIndex; });
    for (auto i = it; i != activeNotes.end(); ++i)
        sink.onNoteOff(i->ctx, i->note);
    activeNotes.erase(it, activeNotes.end());
}
