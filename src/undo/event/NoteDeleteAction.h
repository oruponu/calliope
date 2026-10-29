#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>

class NoteDeleteAction : public juce::UndoableAction
{
public:
    NoteDeleteAction(MidiSequence* seq, int trackIndex, int noteIndex)
        : sequence(seq), trackIdx(trackIndex), noteIdx(noteIndex)
    {
    }

    bool perform() override
    {
        deletedNote = sequence->getTrack(trackIdx).getNote(noteIdx);
        sequence->removeNote(trackIdx, noteIdx);
        return true;
    }

    bool undo() override
    {
        sequence->insertNote(trackIdx, noteIdx, deletedNote);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int trackIdx;
    int noteIdx;
    MidiNote deletedNote;
};
