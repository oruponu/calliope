#pragma once

#include "engine/playback/PlaybackTrackContext.h"
#include "model/track/MidiTrack.h"
#include "model/track/TrackId.h"

// Where chased state lands: a MIDI device channel, or a channel of the plugin a track routes to.
struct ChaseTarget
{
    MidiTrack::OutputDestination destination = MidiTrack::OutputDestination::MidiDevice;
    TrackId routeTarget{};
    int channel = 1;
    bool operator==(const ChaseTarget&) const = default;
};

inline ChaseTarget chaseTargetOf(const PlaybackTrackContext& ctx)
{
    const bool plugin = ctx.destination == MidiTrack::OutputDestination::Plugin;
    return {ctx.destination, plugin ? ctx.routeTarget : TrackId{}, ctx.channel};
}
