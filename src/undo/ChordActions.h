#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

class ChordChangeAction : public juce::UndoableAction
{
public:
    ChordChangeAction(MidiSequence* seq, int tick, int chordRoot, int chordType, int bassRoot, int bassType)
        : sequence(seq), tick(tick), chordRoot(chordRoot), chordType(chordType), bassRoot(bassRoot), bassType(bassType)
    {
    }

    bool perform() override
    {
        before = sequence->getChordChanges();
        sequence->addChordChange(tick, chordRoot, chordType, bassRoot, bassType);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setChordChanges(before);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int tick;
    int chordRoot;
    int chordType;
    int bassRoot;
    int bassType;
    std::vector<ChordChange> before;
};
