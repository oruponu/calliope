#pragma once

#include "model/MidiSequence.h"
#include "undo/event/NoteModification.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

class MultiNoteModifyAction : public juce::UndoableAction
{
public:
    MultiNoteModifyAction(MidiSequence* seq, std::vector<NoteModification> mods) : sequence(seq), mods(std::move(mods))
    {
    }

    bool perform() override
    {
        MidiSequence::ChangeBatch batch(*sequence);
        for (const auto& m : mods)
            sequence->setNote(m.trackIndex, m.noteIndex, m.after);
        return true;
    }

    bool undo() override
    {
        MidiSequence::ChangeBatch batch(*sequence);
        for (const auto& m : mods)
            sequence->setNote(m.trackIndex, m.noteIndex, m.before);
        return true;
    }

    int getSizeInUnits() override { return static_cast<int>(mods.size()); }

private:
    MidiSequence* sequence;
    std::vector<NoteModification> mods;
};
