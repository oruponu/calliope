#pragma once

#include "model/MidiSequence.h"
#include <functional>
#include <juce_data_structures/juce_data_structures.h>
#include <optional>
#include <string>

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
        sequence->notifyTracksChanged();
        return true;
    }

    bool undo() override
    {
        if (beforeChange)
            beforeChange(trackIdx);
        sequence->getTrack(trackIdx).setChannel(oldCh);
        sequence->notifyTracksChanged();
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
        sequence->notifyTracksChanged();
        return true;
    }

    bool undo() override
    {
        sequence->getTrack(trackIdx).setName(oldName);
        sequence->notifyTracksChanged();
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
    explicit TrackAddAction(MidiSequence* seq) : sequence(seq) {}

    bool perform() override
    {
        if (removedTrack)
        {
            sequence->insertTrack(addedIndex, *removedTrack);
            removedTrack.reset();
        }
        else
        {
            sequence->addTrack();
            addedIndex = sequence->getNumTracks() - 1;
        }
        sequence->notifyTracksChanged();
        return true;
    }

    bool undo() override
    {
        removedTrack = sequence->getTrack(addedIndex);
        sequence->removeTrack(addedIndex);
        sequence->notifyTracksChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

    int getAddedIndex() const { return addedIndex; }

private:
    MidiSequence* sequence;
    int addedIndex = -1;
    // Held between undo and redo so redo restores the same id.
    std::optional<MidiTrack> removedTrack;
};

class TrackRemoveAction : public juce::UndoableAction
{
public:
    TrackRemoveAction(MidiSequence* seq, int trackIndex) : sequence(seq), trackIdx(trackIndex) {}

    bool perform() override
    {
        savedTrack = sequence->getTrack(trackIdx);
        sequence->removeTrack(trackIdx);
        sequence->notifyTracksChanged();
        return true;
    }

    bool undo() override
    {
        sequence->insertTrack(trackIdx, savedTrack);
        sequence->notifyTracksChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int trackIdx;
    MidiTrack savedTrack;
};
