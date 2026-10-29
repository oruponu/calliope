#pragma once

#include "model/track/MidiTrack.h"
#include <string>

namespace NewTrack
{
// Named after the row it is added at, so the name stays with the track when the tracks are reordered.
inline MidiTrack make(int index)
{
    MidiTrack track;
    track.setName("Track " + std::to_string(index + 1));
    return track;
}
} // namespace NewTrack
