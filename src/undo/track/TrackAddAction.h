#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <optional>

class TrackAddAction : public juce::UndoableAction
{
public:
    TrackAddAction(MidiSequence* seq, int index, MidiTrack track = {})
        : sequence(seq), addedIndex(index), newTrack(std::move(track))
    {
    }

    bool perform() override
    {
        if (removedTrack)
        {
            sequence->insertTrack(addedIndex, *removedTrack);
            removedTrack.reset();
        }
        else
        {
            sequence->addTrackAt(addedIndex, newTrack);
        }
        return true;
    }

    bool undo() override
    {
        removedTrack = sequence->getTrack(addedIndex);
        sequence->removeTrack(addedIndex);
        return true;
    }

    int getSizeInUnits() override { return 1; }

    int getAddedIndex() const { return addedIndex; }

private:
    MidiSequence* sequence;
    int addedIndex;
    MidiTrack newTrack;
    // Held between undo and redo so redo restores the same id.
    std::optional<MidiTrack> removedTrack;
};
