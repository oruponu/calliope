#pragma once

#include "../model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <algorithm>
#include <map>
#include <vector>

class NoteAddAction : public juce::UndoableAction
{
public:
    NoteAddAction(MidiSequence* seq, int trackIndex, const MidiNote& note)
        : sequence(seq), trackIdx(trackIndex), note(note)
    {
    }

    bool perform() override
    {
        sequence->getTrack(trackIdx).addNote(note);
        addedIndex = sequence->getTrack(trackIdx).getNumNotes() - 1;
        return true;
    }

    bool undo() override
    {
        sequence->getTrack(trackIdx).removeNote(addedIndex);
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
        sequence->getTrack(trackIdx).removeNote(noteIdx);
        return true;
    }

    bool undo() override
    {
        sequence->getTrack(trackIdx).insertNote(noteIdx, deletedNote);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int trackIdx;
    int noteIdx;
    MidiNote deletedNote;
};

class NoteModifyAction : public juce::UndoableAction
{
public:
    NoteModifyAction(MidiSequence* seq, int trackIndex, int noteIndex, const MidiNote& before, const MidiNote& after)
        : sequence(seq), trackIdx(trackIndex), noteIdx(noteIndex), beforeNote(before), afterNote(after)
    {
    }

    bool perform() override
    {
        auto& note = sequence->getTrack(trackIdx).getNote(noteIdx);
        note = afterNote;
        return true;
    }

    bool undo() override
    {
        auto& note = sequence->getTrack(trackIdx).getNote(noteIdx);
        note = beforeNote;
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
        {
            for (int idx : indices)
                deletedNotes.push_back({trackIdx, idx, seq->getTrack(trackIdx).getNote(idx)});
        }
    }

    bool perform() override
    {
        for (const auto& info : deletedNotes)
            sequence->getTrack(info.trackIndex).removeNote(info.noteIndex);
        return true;
    }

    bool undo() override
    {
        for (auto it = deletedNotes.rbegin(); it != deletedNotes.rend(); ++it)
            sequence->getTrack(it->trackIndex).insertNote(it->noteIndex, it->note);
        return true;
    }

    int getSizeInUnits() override { return static_cast<int>(deletedNotes.size()); }

private:
    MidiSequence* sequence;
    std::vector<DeletedNoteInfo> deletedNotes;
};

class MultiNoteAddAction : public juce::UndoableAction
{
public:
    MultiNoteAddAction(MidiSequence* seq, int trackIndex, const std::vector<MidiNote>& notesToAdd)
        : sequence(seq), trackIdx(trackIndex), notes(notesToAdd)
    {
    }

    bool perform() override
    {
        auto& track = sequence->getTrack(trackIdx);
        addedStartIndex = track.getNumNotes();
        for (const auto& note : notes)
            track.addNote(note);
        return true;
    }

    bool undo() override
    {
        auto& track = sequence->getTrack(trackIdx);
        for (int i = static_cast<int>(notes.size()) - 1; i >= 0; --i)
            track.removeNote(addedStartIndex + i);
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

struct VelocityChange
{
    int noteIndex;
    int oldVelocity;
    int newVelocity;
};

class VelocityEditAction : public juce::UndoableAction
{
public:
    VelocityEditAction(MidiSequence* seq, int trackIndex, std::vector<VelocityChange> changes)
        : sequence(seq), trackIdx(trackIndex), changes(std::move(changes))
    {
    }

    bool perform() override
    {
        auto& track = sequence->getTrack(trackIdx);
        for (const auto& c : changes)
            track.getNote(c.noteIndex).velocity = c.newVelocity;
        return true;
    }

    bool undo() override
    {
        auto& track = sequence->getTrack(trackIdx);
        for (const auto& c : changes)
            track.getNote(c.noteIndex).velocity = c.oldVelocity;
        return true;
    }

    int getSizeInUnits() override { return static_cast<int>(changes.size()); }

private:
    MidiSequence* sequence;
    int trackIdx;
    std::vector<VelocityChange> changes;
};
