#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

class MultiNoteAddAction : public juce::UndoableAction
{
public:
    MultiNoteAddAction(MidiSequence* seq, int trackIndex, const std::vector<MidiNote>& notesToAdd)
        : sequence(seq), trackIdx(trackIndex), notes(notesToAdd)
    {
    }

    bool perform() override
    {
        addedStartIndex = sequence->getTrack(trackIdx).getNumNotes();
        MidiSequence::ChangeBatch batch(*sequence);
        for (const auto& note : notes)
            sequence->addNote(trackIdx, note);
        return true;
    }

    bool undo() override
    {
        MidiSequence::ChangeBatch batch(*sequence);
        for (int i = static_cast<int>(notes.size()) - 1; i >= 0; --i)
            sequence->removeNote(trackIdx, addedStartIndex + i);
        return true;
    }

    int getSizeInUnits() override { return static_cast<int>(notes.size()); }

    int getAddedStartIndex() const { return addedStartIndex; }
    int getAddedCount() const { return static_cast<int>(notes.size()); }

private:
    MidiSequence* sequence;
    int trackIdx;
    std::vector<MidiNote> notes;
    int addedStartIndex = 0;
};
