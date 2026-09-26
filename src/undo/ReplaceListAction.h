#pragma once

#include "model/MidiSequence.h"
#include <juce_data_structures/juce_data_structures.h>
#include <utility>
#include <vector>

template <typename T> struct ReplaceListTraits;

template <> struct ReplaceListTraits<TempoChange>
{
    static void set(MidiSequence& seq, std::vector<TempoChange> list) { seq.setTempoChanges(std::move(list)); }
    static void notify(MidiSequence& seq) { seq.notifyTempoChanged(); }
};

template <> struct ReplaceListTraits<TimeSignatureChange>
{
    static void set(MidiSequence& seq, std::vector<TimeSignatureChange> list)
    {
        seq.setTimeSignatureChanges(std::move(list));
    }
    static void notify(MidiSequence& seq) { seq.notifyTimelineMetadataChanged(); }
};

template <> struct ReplaceListTraits<KeySignatureChange>
{
    static void set(MidiSequence& seq, std::vector<KeySignatureChange> list)
    {
        seq.setKeySignatureChanges(std::move(list));
    }
    static void notify(MidiSequence& seq) { seq.notifyTimelineMetadataChanged(); }
};

template <> struct ReplaceListTraits<ChordChange>
{
    static void set(MidiSequence& seq, std::vector<ChordChange> list) { seq.setChordChanges(std::move(list)); }
    static void notify(MidiSequence& seq) { seq.notifyTimelineMetadataChanged(); }
};

template <typename T> class ReplaceListAction : public juce::UndoableAction
{
public:
    ReplaceListAction(MidiSequence* seq, std::vector<T> before, std::vector<T> after)
        : sequence(seq), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override
    {
        ReplaceListTraits<T>::set(*sequence, after);
        ReplaceListTraits<T>::notify(*sequence);
        return true;
    }

    bool undo() override
    {
        ReplaceListTraits<T>::set(*sequence, before);
        ReplaceListTraits<T>::notify(*sequence);
        return true;
    }

    int getSizeInUnits() override { return 1; }

private:
    MidiSequence* sequence;
    std::vector<T> before;
    std::vector<T> after;
};

template <typename T>
void performReplaceList(juce::UndoManager& undoManager, MidiSequence* sequence, const juce::String& transactionName,
                        std::vector<T> before, std::vector<T> after)
{
    undoManager.beginNewTransaction(transactionName);
    undoManager.perform(new ReplaceListAction<T>(sequence, std::move(before), std::move(after)));
}
