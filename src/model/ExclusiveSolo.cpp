#include "model/ExclusiveSolo.h"

namespace ExclusiveSolo
{
void toggle(MidiSequence& sequence, const std::set<int>& indices)
{
    bool alreadyExclusive = true;
    for (int i = 0; i < sequence.getNumTracks(); ++i)
        if (sequence.getTrack(i).isSolo() != indices.contains(i))
            alreadyExclusive = false;

    MidiSequence::ChangeBatch batch(sequence);
    for (int i = 0; i < sequence.getNumTracks(); ++i)
        sequence.setTrackSolo(i, !alreadyExclusive && indices.contains(i));
}
} // namespace ExclusiveSolo
