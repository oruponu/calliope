#pragma once

#include "model/track/MidiTrack.h"
#include "model/track/TrackId.h"

struct PlaybackTrackContext
{
    TrackId trackId{};
    int channel = 1;
    TrackId routeTarget{};
    MidiTrack::OutputDestination destination = MidiTrack::OutputDestination::MidiDevice;
    bool operator==(const PlaybackTrackContext&) const = default;
};
