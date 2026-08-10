#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

class ChordChangeAction : public juce::UndoableAction
{
public:
    ChordChangeAction(MidiSequence* seq, int tick, int chordRoot, int chordType, int bassRoot, int bassType)
        : sequence(seq), tick(tick), chordRoot(chordRoot), chordType(chordType), bassRoot(bassRoot), bassType(bassType)
    {
    }

    bool perform() override
    {
        before = sequence->getChordChanges();
        sequence->addChordChange(tick, chordRoot, chordType, bassRoot, bassType);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setChordChanges(before);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    int tick;
    int chordRoot;
    int chordType;
    int bassRoot;
    int bassType;
    std::vector<ChordChange> before;
};

class ChordAddAction : public juce::UndoableAction
{
public:
    ChordAddAction(MidiSequence* seq, std::vector<ChordChange> before, std::vector<ChordChange> after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        sequence->setChordChanges(after);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setChordChanges(before);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<ChordChange> before;
    std::vector<ChordChange> after;
};

class ChordResizeAction : public juce::UndoableAction
{
public:
    ChordResizeAction(MidiSequence* seq, std::vector<ChordChange> before, std::vector<ChordChange> after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        sequence->setChordChanges(after);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setChordChanges(before);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<ChordChange> before;
    std::vector<ChordChange> after;
};

class ChordMoveAction : public juce::UndoableAction
{
public:
    ChordMoveAction(MidiSequence* seq, std::vector<ChordChange> before, std::vector<ChordChange> after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        sequence->setChordChanges(after);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setChordChanges(before);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<ChordChange> before;
    std::vector<ChordChange> after;
};

class ChordDeleteAction : public juce::UndoableAction
{
public:
    ChordDeleteAction(MidiSequence* seq, std::vector<ChordChange> before, std::vector<ChordChange> after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        sequence->setChordChanges(after);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setChordChanges(before);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<ChordChange> before;
    std::vector<ChordChange> after;
};

class ChordPasteAction : public juce::UndoableAction
{
public:
    ChordPasteAction(MidiSequence* seq, std::vector<ChordChange> before, std::vector<ChordChange> after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        sequence->setChordChanges(after);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setChordChanges(before);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<ChordChange> before;
    std::vector<ChordChange> after;
};
