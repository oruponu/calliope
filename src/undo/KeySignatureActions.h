#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

class KeySignatureChangeAction : public juce::UndoableAction
{
public:
    KeySignatureChangeAction(MidiSequence* seq, int tick, int sharpsOrFlats, bool isMinor)
        : sequence(seq), tick(tick), sharpsOrFlats(sharpsOrFlats), isMinor(isMinor)
    {
    }

    bool perform() override
    {
        before = sequence->getKeySignatureChanges();
        sequence->addKeySignatureChange(tick, sharpsOrFlats, isMinor);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setKeySignatureChanges(before);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int tick;
    int sharpsOrFlats;
    bool isMinor;
    std::vector<KeySignatureChange> before;
};
