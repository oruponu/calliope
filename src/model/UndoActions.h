#pragma once

#include "../model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <algorithm>
#include <functional>
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

class ChannelChangeAction : public juce::UndoableAction
{
public:
    ChannelChangeAction(MidiSequence* seq, int trackIndex, int oldChannel, int newChannel,
                        std::function<void(int)> beforeChange)
        : sequence(seq), trackIdx(trackIndex), oldCh(oldChannel), newCh(newChannel),
          beforeChange(std::move(beforeChange))
    {
    }

    bool perform() override
    {
        if (beforeChange)
            beforeChange(trackIdx);
        sequence->getTrack(trackIdx).setChannel(newCh);
        return true;
    }

    bool undo() override
    {
        if (beforeChange)
            beforeChange(trackIdx);
        sequence->getTrack(trackIdx).setChannel(oldCh);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int trackIdx;
    int oldCh;
    int newCh;
    std::function<void(int)> beforeChange;
};

class TrackRenameAction : public juce::UndoableAction
{
public:
    TrackRenameAction(MidiSequence* seq, int trackIndex, std::string oldName, std::string newName)
        : sequence(seq), trackIdx(trackIndex), oldName(std::move(oldName)), newName(std::move(newName))
    {
    }

    bool perform() override
    {
        sequence->getTrack(trackIdx).setName(newName);
        return true;
    }

    bool undo() override
    {
        sequence->getTrack(trackIdx).setName(oldName);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int trackIdx;
    std::string oldName;
    std::string newName;
};

class TrackAddAction : public juce::UndoableAction
{
public:
    TrackAddAction(MidiSequence* seq, std::function<void(int)> beforeRemove)
        : sequence(seq), beforeRemove(std::move(beforeRemove))
    {
    }

    bool perform() override
    {
        sequence->addTrack();
        addedIndex = sequence->getNumTracks() - 1;
        return true;
    }

    bool undo() override
    {
        if (beforeRemove)
            beforeRemove(addedIndex);
        sequence->removeTrack(addedIndex);
        return true;
    }

    int getSizeInUnits() override { return 1; }

    int getAddedIndex() const { return addedIndex; }

private:
    MidiSequence* sequence;
    std::function<void(int)> beforeRemove;
    int addedIndex = -1;
};

class TrackRemoveAction : public juce::UndoableAction
{
public:
    TrackRemoveAction(MidiSequence* seq, int trackIndex, std::function<void(int)> onDetach,
                      std::function<void(int from, int delta)> onRenumber)
        : sequence(seq), trackIdx(trackIndex), onDetach(std::move(onDetach)), onRenumber(std::move(onRenumber))
    {
    }

    bool perform() override
    {
        savedTrack = sequence->getTrack(trackIdx);
        savedRouteTargets.clear();
        for (int i = 0; i < sequence->getNumTracks(); ++i)
            savedRouteTargets.push_back(sequence->getTrack(i).getRouteTargetTrackIndex());

        if (onDetach)
            onDetach(trackIdx);

        sequence->removeTrack(trackIdx);

        if (onRenumber)
            onRenumber(trackIdx + 1, -1);

        for (int i = 0; i < sequence->getNumTracks(); ++i)
        {
            auto& t = sequence->getTrack(i);
            int rt = t.getRouteTargetTrackIndex();
            if (rt == trackIdx)
                t.setRouteTargetTrackIndex(-1);
            else if (rt > trackIdx)
                t.setRouteTargetTrackIndex(rt - 1);
        }
        return true;
    }

    bool undo() override
    {
        if (onRenumber)
            onRenumber(trackIdx, +1);
        sequence->insertTrack(trackIdx, savedTrack);
        for (int i = 0; i < static_cast<int>(savedRouteTargets.size()); ++i)
            sequence->getTrack(i).setRouteTargetTrackIndex(savedRouteTargets[i]);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int trackIdx;
    std::function<void(int)> onDetach;
    std::function<void(int, int)> onRenumber;
    MidiTrack savedTrack;
    std::vector<int> savedRouteTargets;
};

class TempoChangeAction : public juce::UndoableAction
{
public:
    TempoChangeAction(MidiSequence* seq, int tick, double bpm) : sequence(seq), tick(tick), newBpm(bpm) {}

    bool perform() override
    {
        before = sequence->getTempoChanges();
        sequence->addTempoChange(tick, newBpm);
        return true;
    }

    bool undo() override
    {
        sequence->setTempoChanges(before);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int tick;
    double newBpm;
    std::vector<TempoChange> before;
};

class TempoMoveAction : public juce::UndoableAction
{
public:
    TempoMoveAction(MidiSequence* seq, std::vector<TempoChange> before, std::vector<TempoChange> after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        sequence->setTempoChanges(after);
        return true;
    }

    bool undo() override
    {
        sequence->setTempoChanges(before);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<TempoChange> before;
    std::vector<TempoChange> after;
};

class TimeSignatureChangeAction : public juce::UndoableAction
{
public:
    TimeSignatureChangeAction(MidiSequence* seq, int tick, int num, int den)
        : sequence(seq), tick(tick), numerator(num), denominator(den)
    {
    }

    bool perform() override
    {
        before = sequence->getTimeSignatureChanges();
        sequence->addTimeSignatureChange(tick, numerator, denominator);
        return true;
    }

    bool undo() override
    {
        sequence->setTimeSignatureChanges(before);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int tick;
    int numerator;
    int denominator;
    std::vector<TimeSignatureChange> before;
};

class KeySignatureChangeAction : public juce::UndoableAction
{
public:
    KeySignatureChangeAction(MidiSequence* seq, int tick, int sharpsOrFlats, bool isMinor)
        : sequence(seq), tick(tick), sharpsOrFlats(sharpsOrFlats), isMinor(isMinor)
    {
    }

    bool perform() override
    {
        before = sequence->getKeySignatureChanges();
        sequence->addKeySignatureChange(tick, sharpsOrFlats, isMinor);
        return true;
    }

    bool undo() override
    {
        sequence->setKeySignatureChanges(before);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int tick;
    int sharpsOrFlats;
    bool isMinor;
    std::vector<KeySignatureChange> before;
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
