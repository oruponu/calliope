#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

class TempoChangeAction : public juce::UndoableAction
{
public:
    TempoChangeAction(MidiSequence* seq, int tick, double bpm) : sequence(seq), tick(tick), newBpm(bpm) {}

    bool perform() override
    {
        before = sequence->getTimeline().getTempoChanges();
        addedIndex = sequence->addTempoChange(tick, newBpm);
        sequence->notifyTempoChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setTempoChanges(before);
        sequence->notifyTempoChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

    int getAddedIndex() const { return addedIndex; }

private:
    MidiSequence* sequence;
    int tick;
    double newBpm;
    std::vector<TempoChange> before;
    int addedIndex = -1;
};
