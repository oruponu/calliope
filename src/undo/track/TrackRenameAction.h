#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <string>

class TrackRenameAction : public juce::UndoableAction
{
public:
    TrackRenameAction(MidiSequence* seq, int trackIndex, std::string oldName, std::string newName)
        : sequence(seq), trackIdx(trackIndex), oldName(std::move(oldName)), newName(std::move(newName))
    {
    }

    bool perform() override
    {
        sequence->setTrackName(trackIdx, newName);
        return true;
    }

    bool undo() override
    {
        sequence->setTrackName(trackIdx, oldName);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int trackIdx;
    std::string oldName;
    std::string newName;
};
