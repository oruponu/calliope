#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>

class TrackRemoveAction : public juce::UndoableAction
{
public:
    TrackRemoveAction(MidiSequence* seq, int trackIndex) : sequence(seq), trackIdx(trackIndex) {}

    bool perform() override
    {
        savedTrack = sequence->getTrack(trackIdx);
        sequence->removeTrack(trackIdx);
        return true;
    }

    bool undo() override
    {
        sequence->insertTrack(trackIdx, savedTrack);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int trackIdx;
    MidiTrack savedTrack;
};
