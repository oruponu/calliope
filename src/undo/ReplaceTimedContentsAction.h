#pragma once

#include "model/MidiSequence.h"
#include "model/TimedContents.h"
#include "undo/UndoHistory.h"
#include <juce_data_structures/juce_data_structures.h>
#include <utility>

class ReplaceTimedContentsAction : public juce::UndoableAction
{
public:
    ReplaceTimedContentsAction(MidiSequence* seq, TimedContents before, TimedContents after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        sequence->setTimedContents(after);
        return true;
    }

    bool undo() override
    {
        sequence->setTimedContents(before);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    TimedContents before;
    TimedContents after;
};

inline void performReplaceTimedContents(UndoHistory& undoHistory, MidiSequence* sequence,
                                        const juce::String& transactionName, TimedContents before, TimedContents after)
{
    undoHistory.beginNewTransaction(transactionName);
    undoHistory.perform(new ReplaceTimedContentsAction(sequence, std::move(before), std::move(after)));
}
