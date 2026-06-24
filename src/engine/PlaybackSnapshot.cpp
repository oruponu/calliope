#include "PlaybackSnapshot.h"
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

PlaybackSnapshot PlaybackSnapshot::build(const MidiSequence& seq)
{
    PlaybackSnapshot snap;
    snap.ticksPerQuarterNote = seq.getTicksPerQuarterNote();
    snap.tempoChanges = seq.getTempoChanges();
    std::stable_sort(snap.tempoChanges.begin(), snap.tempoChanges.end(),
                     [](const TempoChange& a, const TempoChange& b) { return a.tick < b.tick; });

    const bool anySolo = seq.isAnySolo();
    const int numTracks = seq.getNumTracks();

    for (int t = 0; t < numTracks; ++t)
    {
        const auto& track = seq.getTrack(t);
        if (track.isMuted())
            continue;
        if (anySolo && !track.isSolo())
            continue;

        PlaybackTrackContext ctx;
        ctx.trackIndex = t;
        ctx.channel = track.getChannel();
        ctx.destination = track.getOutputDestination();
        const int rt = track.getRouteTargetTrackIndex();
        ctx.routeTarget = (rt >= 0 && rt < numTracks) ? rt : t;

        for (const auto& note : track.getNotes())
            snap.notes.push_back({ctx, note});
        for (const auto& event : track.getEvents())
            snap.events.push_back({ctx, event});
    }

    std::stable_sort(snap.notes.begin(), snap.notes.end(), [](const ScheduledNote& a, const ScheduledNote& b)
                     { return a.note.startTick < b.note.startTick; });
    std::stable_sort(snap.events.begin(), snap.events.end(),
                     [](const ScheduledEvent& a, const ScheduledEvent& b) { return a.event.tick < b.event.tick; });

    return snap;
}
