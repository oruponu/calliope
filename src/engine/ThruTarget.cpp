#include "engine/ThruTarget.h"

std::optional<PlaybackTrackContext> resolveThruTarget(const MidiSequence& seq, int activeTrackIndex)
{
    if (activeTrackIndex < 0 || activeTrackIndex >= seq.getNumTracks())
        return std::nullopt;
    if (seq.getTrack(activeTrackIndex).getOutputDestination() == MidiTrack::OutputDestination::None)
        return std::nullopt;
    if (!isTrackAudible(seq, activeTrackIndex))
        return std::nullopt;
    return makePlaybackTrackContext(seq, activeTrackIndex);
}
