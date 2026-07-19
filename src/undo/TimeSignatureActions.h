#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

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
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setTimeSignatureChanges(before);
        sequence->notifyTimelineMetadataChanged();
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

class TimeSignatureMoveAction : public juce::UndoableAction
{
public:
    TimeSignatureMoveAction(MidiSequence* seq, std::vector<TimeSignatureChange> before,
                            std::vector<TimeSignatureChange> after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        sequence->setTimeSignatureChanges(after);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setTimeSignatureChanges(before);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<TimeSignatureChange> before;
    std::vector<TimeSignatureChange> after;
};

class TimeSignatureDeleteAction : public juce::UndoableAction
{
public:
    TimeSignatureDeleteAction(MidiSequence* seq, std::vector<TimeSignatureChange> before,
                              std::vector<TimeSignatureChange> after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        sequence->setTimeSignatureChanges(after);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setTimeSignatureChanges(before);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<TimeSignatureChange> before;
    std::vector<TimeSignatureChange> after;
};

class TimeSignaturePasteAction : public juce::UndoableAction
{
public:
    TimeSignaturePasteAction(MidiSequence* seq, std::vector<TimeSignatureChange> before,
                             std::vector<TimeSignatureChange> after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        sequence->setTimeSignatureChanges(after);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    bool undo() override
    {
        sequence->setTimeSignatureChanges(before);
        sequence->notifyTimelineMetadataChanged();
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<TimeSignatureChange> before;
    std::vector<TimeSignatureChange> after;
};
