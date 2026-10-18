#include "engine/PlaybackProcessor.h"
#include <algorithm>
#include <limits>

void PlaybackProcessor::resetCursors(const PlaybackSnapshot& snap, int tick)
{
    noteCursor = (std::size_t)(std::lower_bound(snap.notes.begin(), snap.notes.end(), tick,
                                                [](const ScheduledNote& n, int t) { return n.note.startTick < t; }) -
                               snap.notes.begin());
    eventCursor = (std::size_t)(std::lower_bound(snap.events.begin(), snap.events.end(), tick,
                                                 [](const ScheduledEvent& e, int t) { return e.event.tick < t; }) -
                                snap.events.begin());
}

void PlaybackProcessor::process(const PlaybackSnapshot& snap, int fromTick, int toTick, PlaybackListener& sink)
{
    while (eventCursor < snap.events.size() && snap.events[eventCursor].event.tick < fromTick)
        ++eventCursor;
    while (noteCursor < snap.notes.size() && snap.notes[noteCursor].note.startTick < fromTick)
        ++noteCursor;

    // Held while sending, so that releaseActiveNotesForTrack cannot send a note-off between a note becoming active
    // and its note-on.
    std::lock_guard<std::mutex> lock(activeNotesMutex);
    constexpr int none = std::numeric_limits<int>::max();
    while (true)
    {
        // Note-offs are due up to and including toTick; events and note-ons only before it. At the same tick,
        // note-offs go first, then events, then note-ons.
        int offTick = none;
        for (const auto& a : activeNotes)
            offTick = std::min(offTick, a.note.endTick());
        if (offTick > toTick)
            offTick = none;
        const int eventTick = eventCursor < snap.events.size() && snap.events[eventCursor].event.tick < toTick
                                  ? snap.events[eventCursor].event.tick
                                  : none;
        const int noteTick = noteCursor < snap.notes.size() && snap.notes[noteCursor].note.startTick < toTick
                                 ? snap.notes[noteCursor].note.startTick
                                 : none;

        const int tick = std::min({offTick, eventTick, noteTick});
        if (tick == none)
            break;
        if (offTick == tick)
        {
            offEndingAt(tick, sink);
        }
        else if (eventTick == tick)
        {
            const auto& se = snap.events[eventCursor++];
            sink.onMidiEvent(se.ctx, se.event);
        }
        else
        {
            const auto& sn = snap.notes[noteCursor++];
            activeNotes.push_back(sn);
            sink.onNoteOn(sn.ctx, sn.note);
        }
    }
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

void PlaybackProcessor::releaseActiveNotesForTrack(TrackId trackId, PlaybackListener& sink)
{
    std::lock_guard<std::mutex> lock(activeNotesMutex);
    auto it = std::stable_partition(activeNotes.begin(), activeNotes.end(),
                                    [&](const ScheduledNote& a) { return a.ctx.trackId != trackId; });
    for (auto i = it; i != activeNotes.end(); ++i)
        sink.onNoteOff(i->ctx, i->note);
    activeNotes.erase(it, activeNotes.end());
}

void PlaybackProcessor::offEndingAt(int tick, PlaybackListener& sink)
{
    auto it = std::stable_partition(activeNotes.begin(), activeNotes.end(),
                                    [&](const ScheduledNote& a) { return a.note.endTick() != tick; });
    for (auto i = it; i != activeNotes.end(); ++i)
        sink.onNoteOff(i->ctx, i->note);
    activeNotes.erase(it, activeNotes.end());
}
