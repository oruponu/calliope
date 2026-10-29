#pragma once

#include "model/MidiSequence.h"
#include <algorithm>
#include <functional>
#include <juce_data_structures/juce_data_structures.h>
#include <map>
#include <vector>

struct DeletedNoteInfo
{
    int trackIndex;
    int noteIndex;
    MidiNote note;
};

class MultiNoteDeleteAction : public juce::UndoableAction
{
public:
    template <typename NoteRefSet>
    MultiNoteDeleteAction(MidiSequence* seq, const NoteRefSet& selectedNotes) : sequence(seq)
    {
        std::map<int, std::vector<int>> byTrack;
        for (const auto& ref : selectedNotes)
            byTrack[ref.trackIndex].push_back(ref.noteIndex);

        for (auto& [trackIdx, indices] : byTrack)
            std::sort(indices.begin(), indices.end(), std::greater<int>());

        for (const auto& [trackIdx, indices] : byTrack)
            for (int idx : indices)
                deletedNotes.push_back({trackIdx, idx, seq->getTrack(trackIdx).getNote(idx)});
    }

    bool perform() override
    {
        MidiSequence::ChangeBatch batch(*sequence);
        for (const auto& info : deletedNotes)
            sequence->removeNote(info.trackIndex, info.noteIndex);
        return true;
    }

    bool undo() override
    {
        MidiSequence::ChangeBatch batch(*sequence);
        for (auto it = deletedNotes.rbegin(); it != deletedNotes.rend(); ++it)
            sequence->insertNote(it->trackIndex, it->noteIndex, it->note);
        return true;
    }

    int getSizeInUnits() override { return static_cast<int>(deletedNotes.size()); }

private:
    MidiSequence* sequence;
    std::vector<DeletedNoteInfo> deletedNotes;
};
