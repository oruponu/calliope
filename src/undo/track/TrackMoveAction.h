#pragma once

#include "model/MidiSequence.h"
#include "model/TrackOrder.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

class TrackMoveAction : public juce::UndoableAction
{
public:
    TrackMoveAction(MidiSequence* seq, std::vector<int> trackOrder) : sequence(seq), order(std::move(trackOrder)) {}

    bool perform() override
    {
        sequence->reorderTracks(order);
        return true;
    }

    bool undo() override
    {
        sequence->reorderTracks(TrackOrder::inverse(order));
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<int> order;
};
