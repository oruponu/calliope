#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>

class NoteModifyAction : public juce::UndoableAction
{
public:
    NoteModifyAction(MidiSequence* seq, int trackIndex, int noteIndex, const MidiNote& before, const MidiNote& after)
        : sequence(seq), trackIdx(trackIndex), noteIdx(noteIndex), beforeNote(before), afterNote(after)
    {
    }

    bool perform() override
    {
        sequence->setNote(trackIdx, noteIdx, afterNote);
        return true;
    }

    bool undo() override
    {
        sequence->setNote(trackIdx, noteIdx, beforeNote);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int trackIdx;
    int noteIdx;
    MidiNote beforeNote;
    MidiNote afterNote;
};
