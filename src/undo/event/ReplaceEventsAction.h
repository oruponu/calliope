#pragma once

#include "model/MidiSequence.h"
#include "model/track/TrackId.h"
#include "undo/UndoHistory.h"
#include <juce_data_structures/juce_data_structures.h>
#include <utility>
#include <vector>

class ReplaceEventsAction : public juce::UndoableAction
{
public:
    ReplaceEventsAction(MidiSequence* seq, TrackId trackId, std::vector<MidiEvent> before, std::vector<MidiEvent> after)
        : sequence(seq), track(trackId), before(std::move(before)), after(std::move(after))
    {
    }

    bool perform() override { return replaceWith(after); }
    bool undo() override { return replaceWith(before); }
    int getSizeInUnits() override { return 1; }

private:
    bool replaceWith(const std::vector<MidiEvent>& events)
    {
        const int index = sequence->indexOf(track);
        if (index < 0)
            return false;
        sequence->setEvents(index, events);
        return true;
    }

    MidiSequence* sequence;
    TrackId track;
    std::vector<MidiEvent> before;
    std::vector<MidiEvent> after;
};

inline void performReplaceEvents(UndoHistory& undoHistory, MidiSequence* sequence, const juce::String& transactionName,
                                 TrackId track, std::vector<MidiEvent> before, std::vector<MidiEvent> after)
{
    undoHistory.beginNewTransaction(transactionName);
    undoHistory.perform(new ReplaceEventsAction(sequence, track, std::move(before), std::move(after)));
}
