#include "engine/PlaybackSnapshot.h"
#include "model/ControllerRamps.h"
#include <algorithm>

double PlaybackSnapshot::getTempoAt(int tick) const
{
    double bpm = 120.0;
    for (const auto& tc : tempoChanges)
    {
        if (tc.tick <= tick)
            bpm = tc.bpm;
        else
            break;
    }
    return bpm;
}

double PlaybackSnapshot::tickAfter(double tick, double elapsedMs) const
{
    double bpm = 120.0;
    auto next = tempoChanges.begin();
    for (; next != tempoChanges.end() && next->tick <= tick; ++next)
        bpm = next->bpm;

    for (; next != tempoChanges.end(); ++next)
    {
        const double msToNext = (next->tick - tick) * 60000.0 / (bpm * ticksPerQuarterNote);
        if (elapsedMs < msToNext)
            break;
        elapsedMs -= msToNext;
        tick = next->tick;
        bpm = next->bpm;
    }
    return tick + elapsedMs * bpm * ticksPerQuarterNote / 60000.0;
}

double PlaybackSnapshot::msBetween(double fromTick, double toTick) const
{
    double bpm = 120.0;
    auto next = tempoChanges.begin();
    for (; next != tempoChanges.end() && next->tick <= fromTick; ++next)
        bpm = next->bpm;

    double ms = 0.0;
    for (; next != tempoChanges.end() && next->tick < toTick; ++next)
    {
        ms += (next->tick - fromTick) * 60000.0 / (bpm * ticksPerQuarterNote);
        fromTick = next->tick;
        bpm = next->bpm;
    }
    return ms + (toTick - fromTick) * 60000.0 / (bpm * ticksPerQuarterNote);
}

PlaybackSnapshot PlaybackSnapshot::build(const MidiSequence& seq, const Take* take)
{
    PlaybackSnapshot snap;
    snap.ticksPerQuarterNote = seq.getTimeline().getTicksPerQuarterNote();
    snap.tempoChanges = seq.getTimeline().getTempoChanges();
    std::stable_sort(snap.tempoChanges.begin(), snap.tempoChanges.end(),
                     [](const TempoChange& a, const TempoChange& b) { return a.tick < b.tick; });
    snap.timeSignatureChanges = seq.getTimeline().getTimeSignatureChanges();

    const int numTracks = seq.getNumTracks();

    for (int t = 0; t < numTracks; ++t)
    {
        if (!isTrackAudible(seq, t))
            continue;
        const auto& track = seq.getTrack(t);

        const PlaybackTrackContext ctx = makePlaybackTrackContext(seq, t);
        snap.contexts.push_back(ctx);

        for (const auto& note : track.getNotes())
            snap.notes.push_back({ctx, note});
        for (const auto& event : expandControllerRamps(track.getEvents()))
            snap.events.push_back({ctx, event});
        if (take == nullptr)
            continue;
        if (const auto recorded = take->find(track.getId()); recorded != take->end())
        {
            for (const auto& note : recorded->second.notes)
                snap.notes.push_back({ctx, note});
            for (const auto& event : recorded->second.events)
                snap.events.push_back({ctx, event});
        }
    }

    std::stable_sort(snap.notes.begin(), snap.notes.end(), [](const ScheduledNote& a, const ScheduledNote& b)
                     { return a.note.startTick < b.note.startTick; });
    std::stable_sort(snap.events.begin(), snap.events.end(),
                     [](const ScheduledEvent& a, const ScheduledEvent& b) { return a.event.tick < b.event.tick; });

    return snap;
}

bool isTrackAudible(const MidiSequence& seq, int trackIndex)
{
    const auto& track = seq.getTrack(trackIndex);
    if (track.isMuted())
        return false;
    return !seq.isAnySolo() || track.isSolo();
}

PlaybackTrackContext makePlaybackTrackContext(const MidiSequence& seq, int trackIndex)
{
    const auto& track = seq.getTrack(trackIndex);
    PlaybackTrackContext ctx;
    ctx.trackId = track.getId();
    ctx.channel = track.getChannel();
    ctx.routeTarget = seq.resolveRouteTarget(trackIndex);
    ctx.destination = track.getOutputDestination();
    return ctx;
}
