#pragma once

#include "model/MidiSequence.h"
#include <algorithm>
#include <functional>
#include <juce_data_structures/juce_data_structures.h>
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

struct NoteModification
{
    int trackIndex;
    int noteIndex;
    MidiNote before;
    MidiNote after;
};

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
        MidiSequence::ChangeBatch batch(*sequence);
        for (const auto& c : changes)
        {
            MidiNote note = sequence->getTrack(trackIdx).getNote(c.noteIndex);
            note.velocity = c.newVelocity;
            sequence->setNote(trackIdx, c.noteIndex, note);
        }
        return true;
    }

    bool undo() override
    {
        MidiSequence::ChangeBatch batch(*sequence);
        for (const auto& c : changes)
        {
            MidiNote note = sequence->getTrack(trackIdx).getNote(c.noteIndex);
            note.velocity = c.oldVelocity;
            sequence->setNote(trackIdx, c.noteIndex, note);
        }
        return true;
    }

    int getSizeInUnits() override { return static_cast<int>(changes.size()); }

private:
    MidiSequence* sequence;
    int trackIdx;
    std::vector<VelocityChange> changes;
};
