#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>

class NoteAddAction : public juce::UndoableAction
{
public:
    NoteAddAction(MidiSequence* seq, int trackIndex, const MidiNote& note)
        : sequence(seq), trackIdx(trackIndex), note(note)
    {
    }

    bool perform() override
    {
        sequence->addNote(trackIdx, note);
        addedIndex = sequence->getTrack(trackIdx).getNumNotes() - 1;
        return true;
    }

    bool undo() override
    {
        sequence->removeNote(trackIdx, addedIndex);
        return true;
    }

    int getSizeInUnits() override { return 1; }

    int getAddedIndex() const { return addedIndex; }

private:
    MidiSequence* sequence;
    int trackIdx;
    MidiNote note;
    int addedIndex = -1;
};
