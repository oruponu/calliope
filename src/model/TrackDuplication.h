#pragma once

#include "model/MidiSequence.h"
#include "model/MidiTrack.h"

namespace TrackDuplication
{
// A copy to pass to MidiSequence::addTrackAt, which gives it its own id. A copy of a track with a plugin
// plays through the same plugin as the original instead of loading another, and a named copy gets the number
// after the highest one in use for that name.
MidiTrack makeCopy(const MidiSequence& sequence, int index);
} // namespace TrackDuplication
