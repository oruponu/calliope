#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

class TempoChangeAction : public juce::UndoableAction
{
public:
    TempoChangeAction(MidiSequence* seq, int tick, double bpm) : sequence(seq), tick(tick), newBpm(bpm) {}

    bool perform() override
    {
        before = sequence->getTempoChanges();
        addedIndex = sequence->addTempoChange(tick, newBpm);
        sequence->notifyTempoChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setTempoChanges(before);
        sequence->notifyTempoChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

    int getAddedIndex() const { return addedIndex; }

private:
    MidiSequence* sequence;
    int tick;
    double newBpm;
    std::vector<TempoChange> before;
    int addedIndex = -1;
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
        sequence->notifyTempoChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setTempoChanges(before);
        sequence->notifyTempoChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<TempoChange> before;
    std::vector<TempoChange> after;
};

class TempoDeleteAction : public juce::UndoableAction
{
public:
    TempoDeleteAction(MidiSequence* seq, std::vector<TempoChange> before, std::vector<TempoChange> after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        sequence->setTempoChanges(after);
        sequence->notifyTempoChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setTempoChanges(before);
        sequence->notifyTempoChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<TempoChange> before;
    std::vector<TempoChange> after;
};

class TempoPasteAction : public juce::UndoableAction
{
public:
    TempoPasteAction(MidiSequence* seq, std::vector<TempoChange> before, std::vector<TempoChange> after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        sequence->setTempoChanges(after);
        sequence->notifyTempoChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setTempoChanges(before);
        sequence->notifyTempoChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<TempoChange> before;
    std::vector<TempoChange> after;
};
